/**
 *
 * Daniel Wright
 * djw67@pitt.edu
 * 
 * CS 449 Project 5
 *
 * Compiled with:
 * gcc client.c -o client -lpthread
 * 
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>

////////////////////////////////
// Global variables, forward declarations, and macros

// Per project instructions, this will always be 1024
#define CHUNK_SIZE 1024

// Wait this many milliseconds after obtaining file mutex
// This may not be necessary, but just be safe for keeping file intact (not corrupt)
#define EXTRA_WAIT 3

// Per project instructions, always just ouput to this file2
#define OUTPUT_FILE "output.txt"

static int __connect_and_get_socket_fp(char* pAddress, int pPort);

////////////////////////////////
// structs

typedef struct download_worker download_worker;
typedef struct download_instance download_instance;

struct download_worker {

	char* ip_addr;
	int port_num;
	int chunk_size;	// In project description, this will be 1024
	int worker_index;
	int socket_fd;
	int socket_is_valid;

	pthread_t current_thread;

	void* buffer;

	// This threads personal mutex
	pthread_mutex_t lock;

	download_instance* thiz_instance;

};

struct download_instance {

	int num_threads;
	download_worker* downloaders;

	FILE* output_file;
	
	int buffer_size;
	void* buffer;

	// Lock used to ensure synchronous 
	pthread_mutex_t file_lock;

	//
	pthread_mutex_t complete_mutex;

	//
	pthread_cond_t complete_cond;

	// The semaphore associated with thread completion
	sem_t completion_count;

};

////////////////////////////////
// Usage Printing

static void print_usage(char* pProgramName) {
	printf("USAGE:\n%s IP port ...\n... denotes that the user may enter additional IP port pairs.\n", pProgramName);
}

////////////////////////////////
// Instance tear down functions

/**
 * 
 * Destroys a current instance of this program.
 * The struct (or any of its fields) CANNOT be accessed
 * after this function has been called on it
 *
 * @param pInstance The instance
 */
static void tear_down_instance(download_instance* pInstance) {

	int i;

	for(i = 0; i < pInstance->num_threads; i++) {

		download_worker* worker = &(pInstance->downloaders[i]);

		free(worker->ip_addr);
		free(worker->buffer);
		
		pthread_mutex_destroy( &(worker->lock) );

	}

	free(pInstance->downloaders);
	free(pInstance->buffer);

	pthread_mutex_destroy( &(pInstance->file_lock) );
	pthread_mutex_destroy( &(pInstance->complete_mutex) );
	pthread_cond_destroy( &(pInstance->complete_cond) );
	sem_destroy( &(pInstance->completion_count) );

	free(pInstance);

}

////////////////////////////////
// File I/O

/**
 * 
 * Checks to see if we need to temporarily write bytes to the file
 * before the worker writes CHUNK_SIZE bytes of data at the specified file offset (from beginning)
 *
 * @param   pWorker    	Worker instance wanting to write data to file
 * @param   pPureOffset The number of bytes offset from the beginning of the file
 *
 * @return The number of bytes needed to write to the file
 *
 * @private
 */
long int __need_to_allocate_bytes(download_worker* pWorker, long int pPureOffset, ssize_t pBytesToWrite) {

	FILE* fp = pWorker->thiz_instance->output_file;

	fseek(fp, 0, SEEK_END);

	long int length = ftell(fp);

	return pPureOffset - length;

}

/**
 * 
 * Write a specific chunk to the output file.
 * If the chunk if past the current EOF, null values will be written to the
 * file to provide a way to write to the file at the moment a thread calls this function
 *
 * @param   pWorker       [description]
 * @param   pChunkIndex   [description]
 * @param   pBytesToWrite [description]
 *
 * @private
 */
static void __write_chunk_to_file(download_worker* pWorker, int pChunkIndex, ssize_t pBytesToWrite) {

	pthread_mutex_lock( &(pWorker->thiz_instance->file_lock) );

	//long int pure_f_offset = (pChunkIndex - 1) * CHUNK_SIZE + pBytesToWrite;
	
	long int pure_f_offset = (pChunkIndex) * CHUNK_SIZE;

	long int bytes_needed = __need_to_allocate_bytes(pWorker, pure_f_offset, pBytesToWrite);

	FILE* fp = pWorker->thiz_instance->output_file;

	while (bytes_needed > 0) {

		int bytes_to_write = bytes_needed < pWorker->thiz_instance->buffer_size ? bytes_needed : pWorker->thiz_instance->buffer_size;
		bytes_needed = bytes_needed - fwrite(pWorker->thiz_instance->buffer, 1, bytes_to_write, fp);

	}

	fseek(fp, pure_f_offset, SEEK_SET);

	//printf("writing %d bytes to file @ offset %d [%d] [chnk indx %d]\n", pBytesToWrite, pure_f_offset, pWorker->worker_index, pChunkIndex);
	
	fwrite(pWorker->buffer, 1, pBytesToWrite, fp);
	fflush(fp);

	pthread_mutex_unlock( &(pWorker->thiz_instance->file_lock) );

}

////////////////////////////////
// Client / Server communicating

/**
 *
 * Get a the next chunk from the server. If 0 is returned from recv,
 * then we know that the EOF has been reached for our spawned thread, so
 * set a field in the struct (socket_is_valid) to false, so we know we can
 * dispose of this job thread.
 *
 * @param   pWorker         [description]
 * @param   pChunkToRequest [description]
 *
 * @return                  [description]
 *
 * @private
 */
static ssize_t __request_next_chunk(download_worker* pWorker, int pChunkToRequest) {

	char str_buffer[128];
	int len = sprintf(str_buffer, "%d", pChunkToRequest);

	int send_res = send(pWorker->socket_fd, str_buffer, 128, 0);

	ssize_t received_bytes = recv(pWorker->socket_fd, pWorker->buffer, CHUNK_SIZE, 0);

	close(pWorker->socket_fd);

	if (received_bytes > 0) {
		pWorker->socket_fd = __connect_and_get_socket_fp(pWorker->ip_addr, pWorker->port_num);
	} else {
		pWorker->socket_is_valid = 0;
	}

	return received_bytes;

}

/**
 *
 * Open a socket connection between the client and server on a thread
 *
 * @param   pAddress [description]
 * @param   pPort    [description]
 *
 * @return           [description]
 *
 * @private
 */
static int __connect_and_get_socket_fp(char* pAddress, int pPort) {

	int sd;
	int ret_val;
	struct hostent* hostaddr;
	struct sockaddr_in servaddr;

	sd = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(pPort);

	hostaddr = gethostbyname(pAddress);

	memcpy(&servaddr.sin_addr, hostaddr->h_addr, hostaddr->h_length);

	ret_val = connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	
	return sd;

}

////////////////////////////////
// Task Thread functions

/**
 * 
 * Called upon completion of __bg_routine().
 * If all threads have completed downloading data, then this function will
 * notify the main thread's condition variable, and wake the main thread up
 *
 * @param   pWorker [description]
 *
 * @private
 */
static void __thread_complete(download_worker* pWorker) {

	// TODO - locking?

	sem_post( &(pWorker->thiz_instance->completion_count) );

	int semVal;

	sem_getvalue(&(pWorker->thiz_instance->completion_count), &semVal);

	if (semVal == pWorker->thiz_instance->num_threads) {
		pthread_cond_signal(&(pWorker->thiz_instance->complete_cond));
	}

}

/**
 *
 * The main function used to download data on a background thread
 *
 * @param   pWorker [description]
 *
 * @private
 */
static void __bg_routine(download_worker* pWorker) {

	int worker_index = pWorker->worker_index;
	int chunk_to_request = worker_index;
	int number_of_workers = pWorker->thiz_instance->num_threads;
	int chunks_written = 0;

	while(1) {

		if (!pWorker->socket_is_valid) {
			break;
		}

		ssize_t rec_count = __request_next_chunk(pWorker, chunk_to_request);

		// Worker thread's buffer now holds the data for the file at the given chunk index
		
		if (rec_count <= 0) {
			break;
		}

		__write_chunk_to_file(pWorker, chunk_to_request, rec_count);

		chunk_to_request += number_of_workers;
		chunks_written++;

	}

	__thread_complete(pWorker);

}

/**
 *
 * The startup routine called by all download threads
 *
 * @param   pInput [description]
 *
 * @private
 */
static void* __startRoutine(void* pInput) {

	download_worker* worker = (download_worker*)pInput;

	// 1) connect to IP at port number
	
	worker->socket_fd = __connect_and_get_socket_fp(worker->ip_addr, worker->port_num);

	// Initially, this is locked by main thread. So this will block
	// UNTIL program is ready to begin processing
	pthread_mutex_lock( &(worker->lock) );

	__bg_routine(worker);

	return NULL;

}

////////////////////////////////
// Argument parsing

/**
 *
 * Checks whether the provided string is
 * a valid IPv4 address
 *
 * @param  pString String representing an IP address,
 * which must not provide any additional text
 * 
 * @param pArgumentIndex the index in which this string was
 * present in the invocation of main()
 * 
 * @return 1 if a valid ip address, 0 otherwise
 */
static int is_ip_addr(char* pString, int pArgumentIndex) {

	int length = strlen(pString);

	// Has to be 3 to be a valid IPv4 address
	int dot_count = 0;

	int i;

	// parse through whole string. must find only numbers and 3 dots ('.')
	for (i = 0; i < length; i++) {

		char c = pString[i];

		if ( (c >= '0') && (c <= '9') ) {
			continue;
		} else if (c == '.') {
			dot_count++;
		} else {
			// Bad character in IP address string
			printf("Unknown character parsed in IP address string '%c' (argument %d)\n", c, pArgumentIndex - 1);
			return 0;

		}

	}

	if (dot_count != 3) {

		printf("Invalid IP address '%s' at argument %d\n", pString, pArgumentIndex - 1);
		return 0;

	} else {
		return 1;
	}

}

/**
 * 
 * Checks whether the provided string is
 * a valid port number (which must be greater than 0)
 * 
 * @param  pString String representing a port number,
 * which must not provide any additional text
 *
 * @param pArgumentIndex the index in which this string was
 * present in the invocation of main()
 * 
 * @return 1 if a valid port number, 0 otherwise
 */
static int is_valid_port(char* pString, int pArgumentIndex) {

	if ( atoi(pString) < 1024 ) {

		// either no number was provided in this argument, or it's just 0. Which is illegal
		printf("Invalid port number assigned (%s) at argument %d [port must be greater than 1024]\n", pString, pArgumentIndex - 1);
		return 0;

	}

	return 1;

}

/**
 * 
 *
 * @param  argc count of arguments provided to main()
 * @param  argv the arguments
 *
 * @return 0 on success, non-zero on invalid arguments
 */
static int check_input_validity(int argc, char* argv[]) {

	// An even amount of arguments were provided.
	// This is invalid, since the first argument in argv[] is ALWAYS going to
	// be the program name itself (at least when invoked via command line), which 
	// means that an IP Address / Port pair cannot be created for the rest of the arguments
	if (argc % 2 == 0) {

		printf("Provide a port number for each IP Address.\n");
		return 1;

	}

	int i;

	for (i = 1; i < argc; i++) {

		if (i % 2) {
		// check if valid IP address
			if (!is_ip_addr(argv[i], i)) {
				return 2;
			}

		} else {
		// check if valid port
			if (!is_valid_port(argv[i], i)) {
				return 3;
			}

		}

	}

	return 0;

}

////////////////////////////////

/**
 * 
 * runs an instance of downloading a file
 * paralellized across numerous ports
 * 
 * @param  pInstance the instance
 *
 * @return 0 on sucess, non-zero on failure
 */
static int run_instance(download_instance* pInstance) {

	int ret_code = 0;

	printf("Beginning download...\n");

	int i;

	pthread_mutex_lock( &(pInstance->complete_mutex) );

	for (i = 0; i < pInstance->num_threads; i++) {
		pthread_mutex_unlock( &(pInstance->downloaders[i].lock) );
	}

	// Now wait for completion
	pthread_cond_wait( &(pInstance->complete_cond), &(pInstance->complete_mutex) );

	FILE* fp = pInstance->output_file;
	fseek(fp, 0, SEEK_END);
	long int loc = ftell(fp);

	printf("Complete!\nWrote %d bytes to file %s.\n", loc, OUTPUT_FILE);

	return ret_code;

}

/**
 *
 * Initialize a new instance to run, but does not begin downloading
 *
 * @param  argc [description]
 * @param  argv [description]
 *
 * @return      [description]
 */
static download_instance* new_instance(int argc, char* argv[]) {

	int number_of_download_threads = (argc - 1) / 2;

	download_instance* ret = (download_instance*)malloc(sizeof(download_instance));
	ret->num_threads = number_of_download_threads;
	ret->downloaders = (download_worker*)malloc(sizeof(download_worker) * number_of_download_threads);
	ret->output_file = fopen(OUTPUT_FILE, "w");
	ret->buffer_size = CHUNK_SIZE * number_of_download_threads;
	ret->buffer = malloc(ret->buffer_size);

	bzero(ret->buffer, CHUNK_SIZE * number_of_download_threads);

	if (!ret->output_file) {
		perror("Failed to open file");
	}

	sem_init( &(ret->completion_count), 0, 0);
	pthread_mutex_init( &(ret->file_lock), NULL);
	pthread_mutex_init( &(ret->complete_mutex), NULL);
	pthread_cond_init( &(ret->complete_cond), NULL);

	// set up downloaders
	int arg_parser = 1;
	int i;

	for (i = 0; i < number_of_download_threads; i++) {

		// Copy the ip string into the struct's own copy
		int str_len = strlen(argv[arg_parser]);
		char* ip_str = malloc(str_len + 1);	// + 1 for NULL
		ip_str[str_len] = '\0';
		memcpy(ip_str, argv[arg_parser++], str_len);

		download_worker worker;

		worker.ip_addr = ip_str;
		worker.worker_index = i;
		worker.socket_is_valid = 1;
		worker.port_num = atoi(argv[arg_parser++]);
		worker.chunk_size = CHUNK_SIZE;
		worker.buffer = malloc(CHUNK_SIZE);

		// Set buffer initially to all 0
		bzero(worker.buffer, CHUNK_SIZE);

		pthread_mutex_init(&(worker.lock), NULL);

		// Lock now, so we can start the background thread now, which will
		// initially try to get the lock. This means that when we are ready
		// to start all threads, the main thread can call unlock(), which will
		// then allow all threads to start
		pthread_mutex_lock(&(worker.lock));
		
		worker.thiz_instance = ret;

		ret->downloaders[i] = worker;

		// Start the thread, but it will be locked until we call unlock() on its personal mutex (lock field)
		pthread_create( &(ret->downloaders[i].current_thread), NULL, __startRoutine, &(ret->downloaders[i]) );

	}

	return ret;

}

////////////////////////////////

int main(int argc, char* argv[]) {

	int validity_code = check_input_validity(argc, argv);

	if (validity_code) {

		print_usage(argv[0]);
		return validity_code;

	}

	download_instance* thz = new_instance(argc, argv);
	int ret_code = run_instance(thz);

	tear_down_instance(thz);

	return ret_code;

}
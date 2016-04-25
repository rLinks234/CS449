#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_SIZE 1024

int main(int argc, char **argv)
{
    int opt;
    int serv_fd;
    int port = -1;
    int flag;
    char filename[256];
    FILE *serv_file = NULL;
    int file_size = 0;
    struct stat stats;
    int num_blocks = 0;
    struct sockaddr_in serv_addr;

    //You must specify a port so that you don't conflict with other people.
    if (argc < 4) {
		printf("Usage: serv -p <port> filename\n");
		exit(0);
    }
    //Get the port from the -p option, courtesy of the getopt library
    while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			//if atoi can't parse your number, you get 0
			//and the first 1024 ports are reserved
			if (port <= 1024) {
				printf("Invalid port specified\n");
				exit(0);
			}
			break;
		default:
			printf("Usage: serv -p <port> filename\n");
			exit(0);
		}
    }

    if (optind == argc) {
		printf("Error: no filename given\n");
		return -1;
    }

    strcpy(filename, argv[optind]);

    //Check to see if the file exists and we can access it
    if (stat(filename, &stats) == -1) {
		perror("File access error\n");
		return -1;
    }
    //We also find out the size of the file from the struct we pass it
    file_size = stats.st_size;

    num_blocks = file_size / BLOCK_SIZE;

    //Is there a partially full last block?
    if (file_size % BLOCK_SIZE) {
		num_blocks++;
    }

    serv_file = fopen(filename, "r");
    if (serv_file == NULL) {
		perror("Error opening file\n");
		return -1;
    }

    serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd == -1) {
		perror("Could not create socket\n");
		return -1;
    }
    //Avoid the problem with rebinding a port too quickly
    flag = 1;
    if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
		perror("Could not set socket option\n");
		return -1;
    }
    //Set up a server
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serv_fd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
		perror("Bind error\n");
		return -1;
    }

    if (listen(serv_fd, 10) == -1) {
		perror("Listen error\n");
		return -1;
    }
    //Here's our server loop.
    //We'll accept a client and serve it the chunk it requested
    while (1) {
		struct sockaddr client_addr;
		int addr_size = sizeof(struct sockaddr);
		int client_fd = accept(serv_fd, (struct sockaddr *) &client_addr, &addr_size);
		char recv_buf[128];
		char send_buf[BLOCK_SIZE];
		int block_idx = -1;
		int bytes_read = 0;
		int amt = 0;

		memset(recv_buf, 0, sizeof(recv_buf) / sizeof(char));

		if (client_fd == -1) {
			perror("Unable to accept client connection\n");
			continue;
		}
		//The first thing our protocol states is that you send the number
		//of the block you want to receive as a string. We receive that and then
		//convert it to an int.
		recv(client_fd, recv_buf, 128, 0);

		block_idx = atoi(recv_buf);
		//printf("Chunk %d being requested\n", block_idx);

		//Request is past the end of file
		if (block_idx >= num_blocks) {
			close(client_fd);
			continue;
		}
		//Seek to the block requested
		if (fseek(serv_file, block_idx * BLOCK_SIZE, SEEK_SET) == -1) {
			perror("Fseek error\n");
			close(client_fd);
			continue;
		}
		//Read the block. bytes_read will be how many bytes were successful.
		//if this is less than BLOCK_SIZE, it means we encountered EOF. We'll
		//be sending less than a whole block back to the client.
		if ((bytes_read = fread(send_buf, 1, BLOCK_SIZE, serv_file)) < 1) {
			perror("Unable to read file\n");
			close(client_fd);
			continue;
		}
		//Send back the chunk. This should be fine to send in just one send()
		//but we might as well do it the "right" way and account for less than
		//the whole block being sent in one call.
		while (amt < bytes_read) {
			int ret = send(client_fd, send_buf + amt, bytes_read - amt, 0);
			if (ret < 0) {
				perror(" Send   failed ");
				close(client_fd);
				continue;
			}
			amt += ret;
		}

		//We're done with this request, so go process the next one.
		close(client_fd);
    }

    return 0;
}

#include <stdio.h>
#include <dlfcn.h>

int main() {

	void *handle;
	void (*my_str_copy)(char *, char *);
	char *error;
	handle = dlopen("mystr.so", RTLD_LAZY);
	
	if(!handle) { //handle == NULL
	
	printf("%s\n", dlerror()); //dlerror gives us a string with the error
		exit(1);
	}

	dlerror(); // Clear any existing error
	my_str_copy = dlsym(handle, "my_strcpy"); //lookup the function by name
	if ((error = dlerror()) != NULL) {

		printf ("%s\n", error);
		exit(1);
	
	}

	//Let’s test it.
	char dest[100];
	char src[] = "Hello World!";
	my_str_copy(dest, src);

}

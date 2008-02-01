#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    char path[256] = {0};
    if (argc > 1)
        strcpy(path, argv[1]);
    else
        strcpy(path, "/usr/lib/libJMPIProviderManager.so");
	void * handle = dlopen(path, RTLD_NOW);
	if (!handle)
	{
		fputs (dlerror(), stderr);
		fputs ("\n", stderr);
		exit(1);		
	}
	dlclose(handle);
}

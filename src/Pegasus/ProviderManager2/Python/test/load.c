#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	void * handle = dlopen("/usr/lib/libPGPythonProviderManager.so", RTLD_NOW);
	if (!handle)
	{
		fputs (dlerror(), stderr);
		fputs ("\n", stderr);
		exit(1);		
	}
	dlclose(handle);
}

#include <private/autogen/config.h>
#include <private/backend.h>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>

char* prefix;

/* Use for scandir filtering */
int 
backends_filter(const struct dirent* backend)
{
	regex_t preg;
	int error;
	char regexp[256];

	strcpy(regexp, prefix);
	strcat(regexp, "[:print:]*");

	error = regcomp(&preg, regexp, REG_ICASE|REG_NOSUB|REG_EXTENDED);
	if (error == 0)
	{
		int match;
		
		match = regexec(&preg, backend->d_name, 0, NULL, 0);
		regfree(&preg);
		if (match == 0)
			return 1;
		else
			return 0;
	}
	else
		/* Regex compilation didn't work */
		return 0;
}



struct hwloc_backend_loaded* 
load(char* path)
{
	struct hwloc_backend_st* (*get_backend)();
	void* handle;
	char* error;

	handle = dlopen(*path, RTLD_LAZY);
	if (!handle)
	{
		fputs(dlerror(), stderr);
		return NULL;
	}

	get_backend = dlsym(handle, "hwloc_get_backend");
	if ((error = dlerror()) != NULL)
	{
		fputs(error, stderr);
		return NULL;
	}

	struct hwloc_backends_loaded* backend_loaded = malloc(sizeof(struct hwloc_backends_loaded));
	backend_loaded->handle = handle;
	backend_loaded->backend = get_backend();
	backend_loaded->next = NULL;

	return backend_loaded;
}



struct hwloc_backends_loaded* 
hwloc_backend_load(char* path, const char* backend_prefix)
{
	struct dirent** namelist;
	int n_backends; /* # of corresponding backends found */
	char* backend_path;
	struct hwloc_backends_loaded* backends_loaded = NULL;
	struct hwloc_backends_loaded* backend = NULL;
	struct hwloc_backends_loaded* buff = NULL;

	prefix = malloc(sizeof(backend_prefix));
	prefix = backend_prefix;

	/* backends_filter returns non-zero if backend name matches with backend_prefix */
	n_backends = scandir(*path, &namelist, backends_filter, 0);

	for (int i = 0 ; i < n_backends ; i++)
	{
		backend_path = malloc(strlen(path)*sizeof(char)+sizeof(namelist[i]->d_name)+1);
		strcpy(backend_path, path);
		strcat(backend_path, "/");
		strcat(backend_path, namelist[i]->d_name);

		if ((backend = load(backend_path)) != NULL)
		{
			if (i == 0)
				backends_loaded = backend;
				buff = backend;
			else
			{
				buff->next = backend;
				buff = backend;
			}
		}
	}

	free(backend_path);

	return backends_loaded;
}



void
hwloc_backend_unload(struct hwloc_backends_loaded* backends_loaded)
{
	if (backends_loaded != NULL)
	{
		struct hwloc_backends_loaded* buff;
		struct hwloc_backends_loaded* old;

		buff = backends_loaded;

		do {
			old = buff;
			buff = old->next;

			dlclose(old->handle);
			free(old);
		} while (buff != NULL);
	}

	free(prefix);
}

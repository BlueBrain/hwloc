#include <hwloc/backend.h>

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>

static char* prefix;
int backends_filter(const struct dirent* backend);
struct hwloc_backends_loaded* load(char* path);


/* Use for scandir filtering */
int 
backends_filter(const struct dirent* backend)
{
	regex_t preg;
	int error;
	char regexp[256];

	strcpy(regexp, prefix);
	strcat(regexp, "[[:print:]]*so$");

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



struct hwloc_backends_loaded* 
load(char* path)
{
	struct hwloc_backend_st* (*get_backend)(void);
	void* handle;
	char* error;
	struct hwloc_backends_loaded* backend_loaded;

	handle = dlopen(path, RTLD_LAZY);
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

	backend_loaded = malloc(sizeof(struct hwloc_backends_loaded));
	backend_loaded->handle = handle;
	backend_loaded->backend = get_backend();
	backend_loaded->next = NULL;

	return backend_loaded;
}

static struct hwloc_backends_loaded*
browse_and_load(char* path, char* backend_prefix)
{
	struct dirent** namelist;
	int n_backends = 0; /* # of corresponding backends found */
	char* backend_path = NULL;
	struct hwloc_backends_loaded* backends_loaded = NULL;
	struct hwloc_backends_loaded* backend = NULL;
	struct hwloc_backends_loaded* buff = NULL;
	int i;
	
	/* backends_filter returns non-zero if backend name matches with backend_prefix */
	n_backends = scandir(path, &namelist, backends_filter, 0);

	for (i = 0 ; i < n_backends ; i++)
	{
		backend_path = malloc(strlen(path)*sizeof(char)+sizeof(namelist[i]->d_name)+sizeof(char));
		strcpy(backend_path, path);
		strcat(backend_path, "/");
		strcat(backend_path, namelist[i]->d_name);
	
		fprintf(stderr, "**backend.c: backend_path -> %s\n", backend_path);

		if ((backend = load(backend_path)) != NULL)
		{
			if (i == 0)
			{
				backends_loaded = backend;
				buff = backend;
			}
			else
			{
				buff->next = backend;
				buff = backend;
			}
		}
		free(backend_path);
		backend_path = NULL;
	}

	return backends_loaded;
}

struct hwloc_backends_loaded* 
hwloc_backend_load(char* path, char* hwloc_plugin_dir, char* backend_prefix)
{
	struct hwloc_backends_loaded* backends_loaded = NULL;
	struct hwloc_backends_loaded* plugin = NULL;
	struct hwloc_backends_loaded* buff = NULL;
	
	prefix = backend_prefix;

	fprintf(stderr, "**backend.c: prefix = %s\n", prefix); 

	backends_loaded = browse_and_load(path, backend_prefix);
	if(hwloc_plugin_dir)
	{
		plugin = browse_and_load(hwloc_plugin_dir, backend_prefix);

		if(backends_loaded)
		{
			buff = backends_loaded;
			while(buff->next)
				buff = buff->next;

			buff->next = plugin;

			backends_loaded = buff;
		}
		else
			backends_loaded = plugin;
	}

	prefix=NULL;
	
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
}

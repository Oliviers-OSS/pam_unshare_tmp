/*
 * parameters.c
 *
 *  Created on: 29 juil. 2019
 *      Author: oc
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parameters.h"
#include "ini.h"
#include "debug.h"

#include <sys/mount.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define UNIT(n,v) X(n,v)
#define UNITS_TABLE \
		UNIT("kio",(1<<10)) \
		UNIT("Mio",(1<<20)) \
		UNIT("Gio",(1<<30)) \
		UNIT("ko",1000) \
		UNIT("Mo",(1000*1000)) \
		UNIT("Go",(1000*1000*1000))

static inline size_t size_parser(const char *value) {
	errno = 0;
	char *endptr = NULL;
	size_t size = strtoul(value,&endptr,0);
	if (*endptr != '\0') {
		while(isspace(*endptr)) { endptr++;};
#define X(n,v) if (strncmp(n,endptr,strlen(n)) == 0) size *= v; else
		UNITS_TABLE
#undef X
		{
			errno = EINVAL;
			size = 0;
		};
	}
	return size;
}
#undef UNITS_TABLE
#undef UNIT

#define MO(x) X(x)
#define MOUNT_OPTIONS_TABLE \
	MO(NOEXEC) \
	MO(NODEV) \
	MO(NOATIME) \
	MO(NODIRATIME) \
	MO(NOSUID) \
	MO(PRIVATE)

static inline int mount_options_parser(pam_handle_t *pamh,const char *options, unsigned long *mountflags) {
	int error = EXIT_SUCCESS;
	char *str = (char *)alloca(strlen(options)+1);
	strcpy(str,options);
	register char *token = strtok (str," ,;");
	*mountflags = 0;
	while (token != NULL) {
		if (strcmp(token,"ro") == 0) {
			*mountflags |= MS_RDONLY;
		}
		else if (strcmp(token,"rw") == 0) {
			//*mountflags &= ~MS_RDONLY;
		}

#define X(x) else if (strcasecmp(TO_STRING(x),token) == 0) { *mountflags |= MS_##x; }
		MOUNT_OPTIONS_TABLE
		else { error = EINVAL; ERROR_MSG("Invalid mount option %s",str); break; }
#undef X

		token = strtok (NULL," ,;");
	}

	return error;
}
#undef MOUNT_OPTIONS_TABLE
#undef MO

static inline int configfile_read_parameter(const char* section, const char* name,const char* value,userParameters *params) {
	int error = EXIT_SUCCESS;
	pam_handle_t *pamh = params->pamh;
	if ((strcmp(name, "size") == 0)) {
		const size_t size = size_parser(value);
		if (likely(size)) {
			params->size = size;
			DEBUG_VAR(params->size,"%zu");
		} else {
			WARNING_MSG("Invalid size parameter %s for user %s",value,section);
		}
	} else if (strcmp(name, "options") == 0) {
		unsigned long mountflags = 0;
		error = mount_options_parser(pamh,value,&mountflags);
		if (EXIT_SUCCESS == error) {
			params->mountflags = mountflags;
			DEBUG_VAR(params->mountflags,"0x%X");
		} else {
			WARNING_MSG("Invalid mount options parameter %s for user %s",value,section);
			error = EXIT_SUCCESS;
		}
	} else if (strcmp(name, "dirmode") == 0) {
		params->dirmode = (mode_t)strtoul(value,NULL,0);
		DEBUG_VAR(params->dirmode,"0%o");
	} else if (strcmp(name, "unshare_ipc") == 0) {
		params->unshare_ipc = ((value[0] != 'n') && (value[0] != 'N') && (value[0] != '0'))?true:false;
		DEBUG_VAR_BOOL(params->unshare_ipc);
	} else {
		WARNING_MSG("Unknown parameter %s = %s in section %s",name,value,section);
	}
	return error;
}

static int configfile_line_handler(void* user, const char* section, const char* name,const char* value) {
	userParameters *params = (userParameters*)user;
	pam_handle_t *pamh = params->pamh;
	int error = EXIT_SUCCESS;

	if (strcmp(section, params->username) == 0) {
		error = configfile_read_parameter(section,name,value,params);
	} else if ('\0' == section[0]) {
		error = configfile_read_parameter("(default)",name,value,params);
	} else if (strncmp("group",section,strlen("group")) == 0) {
		DEBUG_MARK;
		register const char *groupname = strchr(section,'=');
		if (likely(groupname)) {
			groupname++;
			DEBUG_VAR(groupname,"%s");
			if (strcmp(groupname,params->groupname) == 0) {
				error = configfile_read_parameter(section,name,value,params);
			}
		}
	}

	return (EXIT_SUCCESS == error); // Warning stop on error !
}


int get_user_parameters(const char *configFile, userParameters *params) {
	int error = ini_parse(configFile, configfile_line_handler, (void*)params);
	pam_handle_t *pamh = params->pamh;
	if (unlikely(error != 0)) {
		if (likely(error > 0)) {
			ERROR_MSG("Error  at line %d in configuration file %s",error,configFile);
			error = EINVAL;
		} else {
			error = errno = -error;
			ERROR_MSG("Error %d (%m) reading configuration file %s",error,configFile);
		}
	}
	DEBUG_VAR(error,"%d");
	return error;
}



/*
 * pam_unshare_tmp.c
 *
 *  Created on: 29 juil. 2019
 *      Author: oc
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif //_GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parameters.h"
#include "debug.h"

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <string.h>
#include <alloca.h>
#include <malloc.h>
#include <limits.h>
#include <utmpx.h>

// man 5 proc:  32768 <= pid <= PID_MAX_LIMIT = 2^22 = 4194304,
// (cat /proc/sys/kernel/pid_max to get its current value)
#define PID_MAX_SIZE (strlen("4194304"))

#define OBTAIN(item, value, default_value)  do {                \
		(void) pam_get_item(pamh, item, &value);                   \
		value = value ? value : default_value ;                    \
} while (0)

#ifdef _DEBUG_
static inline void log_items(pam_handle_t *pamh, const char *function)
{
	const void *service=NULL, *user=NULL, *terminal=NULL, *rhost=NULL, *ruser=NULL;
	setlogmask(LOG_UPTO(LOG_DEBUG));
	OBTAIN(PAM_SERVICE, service, "<unknown>");
	OBTAIN(PAM_TTY, terminal, "<unknown>");
	OBTAIN(PAM_USER, user, "<unknown>");
	OBTAIN(PAM_RUSER, ruser, "<unknown>");
	OBTAIN(PAM_RHOST, rhost, "<unknown>");

	pam_syslog(pamh, LOG_NOTICE,
			"PAM_UNSHARE_TMP function=[%s] service=[%s] terminal=[%s] user=[%s]"
			" ruser=[%s] rhost=[%s]\n", function,
			(const char *) service, (const char *) terminal,
			(const char *) user, (const char *) ruser,
			(const char *) rhost);
}
#else // _DEBUG_
#define log_items(pamh,function)
#endif // _DEBUG_

static inline bool is_number(const char *string) {
	register int c = 0;
	register const char *s = string;
	do {
		c = *s;
		++s;
	} while((c != '\0') && (isdigit(c)));

	return ('\0' == c);
}

#ifdef _DEBUG_
static int print_cmdline(pam_handle_t *pamh,const char *pid) {
	int error = EXIT_SUCCESS;
#define PROC_CMDLINE_PATH "/proc/%s/cmdline"
	const size_t length = strlen(pid) + strlen(PROC_CMDLINE_PATH) + 1;
	char *cmdline = (char *)alloca(length);
	const size_t l = (size_t)sprintf(cmdline,PROC_CMDLINE_PATH,pid);
	ASSERT(l <= length);
	int fd = open(cmdline,O_RDONLY);
	if (likely(fd != -1)) {
		char buffer[4096];
		ssize_t n = read(fd,buffer,sizeof(buffer));
		if (n > 0) {
			INFO_MSG("cmdline = %s",buffer);
		} else {
			error = errno;
			ERROR_MSG("read %s error %d (%m)",cmdline,error);
		}
		close(fd);
		fd = -1;
	} else {
		error = errno;
		ERROR_MSG("open %s error %d (%m)",cmdline,error);
	}
#undef PROC_CMDLINE_PATH
	return error;
}
#else // _DEBUG_
#define print_cmdline(pamh,pid)
#endif //_DEBUG_

static inline bool filtered_exe(const char *program_name) {
	bool filtered = false;
	const char * filtered_names[] = {"/lib/systemd/systemd"};
	register const char ** name = filtered_names;
	register const char ** const end = name + (sizeof(filtered_names) / sizeof(filtered_names[0]));
	for(;name != end;name++) {
		if (strcmp(*name,program_name) == 0) {
			filtered = true;
			break;
		}
	}
	return filtered;
}

static int proc_filter(pam_handle_t *pamh,const char *pid) {
	int error = EXIT_SUCCESS;
#define PROC_EXE_PATH "/proc/%s/exe"
	const size_t proc_length = PID_MAX_SIZE + strlen(PROC_EXE_PATH) + 1;
	char exe[proc_length];
	const size_t l = (size_t)sprintf(exe,PROC_EXE_PATH,pid);
	ASSERT(l <= proc_length);
	struct stat status;
	if (likely(lstat(exe,&status) == 0)) {
		size_t exe_length = (size_t)((status.st_size)?(status.st_size + 1): PATH_MAX);
		char *exe_name = (char *) alloca(exe_length);
		ssize_t n = readlink(exe,exe_name,exe_length);
		if (likely(n > 0)) {
			exe_name[n] = '\0';
			const bool filtered = filtered_exe(exe_name);
			DEBUG_MSG("PID %s, exec = %s : filtered = %s",pid,exe_name,(filtered==true)?"true":"false");
			if (unlikely(filtered)) {
				error = ENOENT;
			}
		} else {
			error = errno;
			ERROR_MSG("readlink %s error %d (%m)",exe,error);
		}
	} else {
		error = errno;
		ERROR_MSG("stat %s error %d (%m)",exe,error);
	}

	DEBUG_VAR(error,"%d");
#undef PROC_EXE_PATH
	return error;
}

static inline int move_to_uid_namespace(pam_handle_t *pamh,const char *ns_path,const int ns_type) {
	int error = EXIT_SUCCESS;
	int nd = open(ns_path,O_RDONLY);
	if (likely(nd != -1)) {
		// Join that namespace
		if (unlikely(setns(nd,ns_type) != 0)) {
			error = errno;
			ERROR_MSG("setns %s error %d (%m)",ns_path,error);
		}
		close(nd);
		nd = -1;
	} else {
		error = errno;
		ERROR_MSG("open %s error %d (%m)",ns_path,error);
	}
	return error;
}
static int move_to_uid_namespaces(pam_handle_t *pamh,const uid_t uid) {
	int error = ENOENT;
	const char * const directory = "/proc";
	DIR *dir = opendir(directory);
	if (likely(dir)) {
		struct dirent *entry = NULL;
		while ((entry = readdir(dir))) {
			// filter on PID
			const char * const pid = entry->d_name;
			if (likely((DT_DIR == entry->d_type) && (is_number(pid)))) {
				int fd = dirfd(dir);
				if (likely(fd != -1)) {
					struct stat status;
					if (likely(fstatat(fd,pid,&status,AT_NO_AUTOMOUNT) == 0)) {
						if (unlikely(uid == status.st_uid)) {
							DEBUG_MSG("FOUND ONE (%s)!\n",pid);
							print_cmdline(pamh,pid);
							error = proc_filter(pamh,pid);
							if (likely(EXIT_SUCCESS == error)) {
								const size_t length = PID_MAX_SIZE + strlen("/proc/%s/ns/mnt"); // same size for ipc
								char ns_path[length];

								// mount
								size_t n = (size_t)sprintf(ns_path,"/proc/%s/ns/mnt",pid);
								ASSERT(n <= length);
								error = move_to_uid_namespace(pamh,ns_path,CLONE_NEWNS);
								if (likely(EXIT_SUCCESS == error)) {
									DEBUG_MSG("Move to user's private mnt namespace successfully done");
									// ipc
									n = (size_t)sprintf(ns_path,"/proc/%s/ns/ipc",pid);
									ASSERT(n <= length);
									error = move_to_uid_namespace(pamh,ns_path,CLONE_NEWIPC);
									if (likely(EXIT_SUCCESS == error)) {
										DEBUG_MSG("Move to user's private IPC namespace successfully done");
										break;
									} else { // else try to find another one
										WARNING_MSG("Move to IPC namespace error %d(%m)",error);
									}
								} else {
									WARNING_MSG("Move to mnt namespace error %d(%m)",error);
								}
							} // (likely(EXIT_SUCCESS == proc_filter))
						} // (unlikely(uid == status.st_uid))
					} else {
						error = errno;
						ERROR_MSG("fstatat /proc/%s error %d (%m)",pid,error);
					}
				} else {
					error = errno;
					ERROR_MSG("dirfd %s error %d (%m)",directory,error);
				}
			} //((DT_DIR == entry->d_type) && (is_number(pid)))
		} //while (entry = readdir(dir))
		closedir(dir);
		dir = NULL;
	} else {
		error = errno;
		ERROR_MSG("opendir %s error %d (%m)",directory,error);
	}

	DEBUG_VAR(error,"%d");
#undef PROC_MOUNT_NS_MNT_PATH
	return error;
}

static inline int make_tmpfs_volume(const char *root,const char *name,const userParameters *params) {
	int error = EXIT_SUCCESS;
	const pam_handle_t *pamh = params->pamh;
	const size_t size = params->size;
	const unsigned long mountflags = params->mountflags;
	const mode_t dirmode = params->dirmode;
	DEBUG_VAR(size,"%zu");
	DEBUG_VAR(mountflags,"0x%lX");
	DEBUG_VAR(dirmode,"0%o");
	if (likely(chdir(root) == 0 )) {
		if (unlikely(mkdir(name,dirmode) != 0)) {
			error = errno;
			if (likely(EEXIST == error)) {
				INFO_MSG("Directory %s/%s already exist",root,name);
				error = EXIT_SUCCESS;
			} else {
				ERROR_MSG("mkdir %s/%s error %d (%m)",root,name,error);
			}
		}

		if (likely(EXIT_SUCCESS == error)) {
			char parameters[127];
			sprintf(parameters,"size=%zu",size);
			if (unlikely(mount("tmpfs", name, "tmpfs", mountflags, parameters) == -1)) {
				error = errno;
				ERROR_MSG("mount tmpfs %s/%s flags = 0x%luX parameters = %s error %d (%m)",root,name,mountflags,parameters,error);
			} else {
				DEBUG_MSG("mount tmpfs %s/%s flags = 0x%luX parameters = %s done",root,name,mountflags,parameters);
			}
		}
	} else {
		error = errno;
		ERROR_MSG("chdir %s error %d (%m)",root,error);
	}
	DEBUG_VAR(error,"%d");
	return error;
}

static inline int umount_tmpfs_volume(pam_handle_t *pamh,const char *root,const char *name,const int umountflags/* = MNT_DETACH | UMOUNT_NOFOLLOW*/) {
	int error = EXIT_SUCCESS;
	if (likely(chdir(root) == 0 )) {
		if (unlikely(umount2(name,umountflags) != 0)) {
			error = errno;
			ERROR_MSG("umount2 %s/%s 0x%X error %d (%m)",root,name,umountflags,error);
		}
	} else {
		error = errno;
		ERROR_MSG("chdir %s error %d (%m)",root,error);
	}
	return error;
}

static int set_user_var_tmp(pam_handle_t *pamh,const uid_t uid, const gid_t gid) {
	int error = EXIT_SUCCESS;
#define VAR_TMP_USER_PATH "/var/.tmp_%u"
	size_t length = 5 + strlen(VAR_TMP_USER_PATH) + 1;
	char *var_tmp_user_path = (char *)alloca(length);
	const size_t n = (size_t)sprintf(var_tmp_user_path,VAR_TMP_USER_PATH,uid);
	ASSERT(n < length);
	// create user's /var/tmp directory (if needed)
	const mode_t mode = S_IRWXU;
	if (unlikely(mkdir(var_tmp_user_path,mode) == 0)) {
		if (unlikely(chown(var_tmp_user_path, uid, gid) != 0)) {
			error = errno;
			ERROR_MSG("mkdir %s error %d (%m)",var_tmp_user_path,error);
		}
	} else {
		error = errno;
		if (likely(EEXIST == error)) {
			error = EXIT_SUCCESS;
		} else {
			ERROR_MSG("mkdir %s error %d (%m)",var_tmp_user_path,error);
		}
	}

	// then mount user's /var/tmp to /var/tmp
	if (likely(EXIT_SUCCESS == error)) {
		if (unlikely(mount(var_tmp_user_path,"/var/tmp",NULL,MS_BIND,NULL) != 0)) {
			error = errno;
			ERROR_MSG("mount %s -> /var/tmp error %d (%m)",var_tmp_user_path,error);
		} else {
			DEBUG_MSG("mount %s -> /var/tmp done",var_tmp_user_path);
		}
	}
#undef VAR_TMP_USER_PATH
	DEBUG_VAR(error,"%d");
	return error;
}

static int move_to_user_namespace(pam_handle_t *pamh,const char *username,const char *configfile) {
	int error = EXIT_SUCCESS;

	long int n = sysconf(_SC_GETPW_R_SIZE_MAX);
	size_t pwd_buffer_size = 16384;
	if (likely(n != -1)) {
		pwd_buffer_size = (size_t)n;
	} else {
		INFO_MSG("sysconf _SC_GETPW_R_SIZE_MAX error");
	}
	DEBUG_VAR(pwd_buffer_size,"%zu");

	char *pwd_buffer = (char *)malloc((size_t)pwd_buffer_size);
	if (likely(pwd_buffer)) {
		struct passwd entry,*result = NULL;
		error = getpwnam_r(username,&entry,pwd_buffer,pwd_buffer_size,&result);
		if (likely(result)) {
			//const pid_t pid = getpid();
			//DEBUG_VAR(pid,"current %u");
			// try to find already mounted volume for this user
			const uid_t uid = entry.pw_uid;
			error = move_to_uid_namespaces(pamh,uid);
			if (unlikely(ENOENT == error)) {
				INFO_MSG("Not running process found for this user: creating its running namespaces...");

				n = sysconf(_SC_GETGR_R_SIZE_MAX);
				size_t grp_buffer_size = 2048;
				if (likely(n != -1)) {
					grp_buffer_size = (size_t)n;
				}
				DEBUG_VAR(grp_buffer_size,"%zu");

				char *grp_buffer = (char *)malloc((size_t)grp_buffer_size);
				if (likely(grp_buffer)) {
					struct group grp, *grp_result = NULL;
					error = getgrgid_r(entry.pw_gid,&grp,grp_buffer,grp_buffer_size,&grp_result);
					if (likely(grp_result)) {
						// get user's parameters
						userParameters params = {pamh,username,grp.gr_name,DEFAULT_SIZE,DEFAULT_MOUNT_OPTIONS,DEFAULT_DIR_MODE,false};
						get_user_parameters(configfile,&params);
						const int unshare_flags = (params.unshare_ipc)?(CLONE_NEWIPC|CLONE_NEWNS):CLONE_NEWNS;
						DEBUG_VAR(unshare_flags,"0x%X");
						if (unshare(unshare_flags) == 0) { // New FS / mount namespace
							// create a new private mount namespace
							if (likely(mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL) == 0)) {
								// set its /tmp directory
								make_tmpfs_volume("/","tmp",&params); // error already printed, try to go on

								error = set_user_var_tmp(pamh,uid,entry.pw_gid);

							} else {
								error = errno;
								ERROR_MSG("mount / MS_REC | MS_PRIVATE error %d",error);
							}
						} else {
							error = errno;
							CRIT_MSG("unshare 0x%X error %d (%m)",unshare_flags,error);
						}
					} else {
						if (likely(EXIT_SUCCESS == error)) {
							error = ENOENT;
						}
						errno = error;
						ERROR_MSG("getgrgid_r %u error %d (%m)",entry.pw_gid,error);
					}
					free(grp_buffer);
					grp_buffer = NULL;
				} else {
					error = ENOMEM;
					ERROR_MSG("Failed to allocate %zu bytes for group strings",grp_buffer_size);
				}
			} // error (if any) already printed
		} else {
			if (likely(EXIT_SUCCESS == error)) {
				error = ENOENT;
			}
			errno = error;
			ERROR_MSG("getpwnam %s error %d (%m)",username,error);
		}
		free(pwd_buffer);
		pwd_buffer = NULL;
	} else {
		error = ENOMEM;
		ERROR_MSG("Failed to allocate %zu bytes for passwd strings",pwd_buffer_size);
	}

	DEBUG_VAR(error,"%d");
	return error;
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags , int argc , const char **argv )
{
	int error = PAM_SUCCESS;
	log_items(pamh, __FUNCTION__);
	const char *username=NULL;

	(void)flags;
	if (likely(pam_get_item(pamh, PAM_USER,(const void**)&username) == PAM_SUCCESS)) {
		DEBUG_VAR(username,"%s");
#ifndef _ROOT_USER_ENABLED_
		if (likely(strcmp(username,"root") != 0)) {
#endif // _ROOT_USER_ENABLED_
			const char *configFile = DEFAULT_CONFIGURATION_FILE;
			if (unlikely(argc)) {
				configFile = argv[0];
			}
			error = move_to_user_namespace(pamh,username,configFile);
#ifndef _ROOT_USER_ENABLED_
		} else {
			INFO_MSG("Disabled for root");
		}
#endif // _ROOT_USER_ENABLED_
	}
	DEBUG_VAR(error,"%d");
	return error;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags , int argc , const char **argv )
{
	int error = PAM_SUCCESS;
	const char *username=NULL;
	log_items(pamh, __FUNCTION__);

	(void)flags;
	(void)argc;
	(void)argv;
	if (likely(pam_get_item(pamh, PAM_USER,(const void**)&username) == PAM_SUCCESS)) {
		DEBUG_VAR(username,"%s");
#ifndef _ROOT_USER_ENABLED_
		if (likely(strcmp(username,"root") != 0)) {
#endif // _ROOT_USER_ENABLED_
			struct utmpx *entry = NULL;
			register unsigned int nb_remaining = 0;

			utmpxname(UTMPX_FILE);
			setutxent();
			while((entry = getutxent())) {
				if ((USER_PROCESS == entry->ut_type) && (strcmp(entry->ut_user,username) == 0)) {
					++nb_remaining;
				}
			}
			endutxent();
			DEBUG_VAR(nb_remaining,"%u");
			if (unlikely(0 == nb_remaining)) {
				error = umount_tmpfs_volume(pamh,"/","/tmp",MNT_DETACH | UMOUNT_NOFOLLOW);
			}
#ifndef _ROOT_USER_ENABLED_
		} else {
			INFO_MSG("Disabled for root");
		}
#endif // _ROOT_USER_ENABLED_
	}
	DEBUG_VAR(error,"%d");
	return error;
}

#ifdef PAM_STATIC

/* static module data */

struct pam_module _pam_unshare_tmp_modstruct = {
		"pam_unshare_tmp", //name
		NULL, // pam_sm_authenticate
		NULL, // pam_sm_setcred
		NULL, // pam_sm_acct_mgmt
		pam_sm_open_session, // pam_sm_open_session
		pam_sm_close_session, // pam_sm_close_session
		NULL // pam_sm_chauthtok
};
#endif //PAM_STATIC

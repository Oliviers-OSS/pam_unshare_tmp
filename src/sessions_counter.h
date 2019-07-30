/*
 * sessions_counter.h
 *
 *  Created on: 30 juil. 2019
 *      Author: oc
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef _SESSIONS_COUNTER_H_
#define _SESSIONS_COUNTER_H_

#include "debug.h"

#ifndef PAM_SM_SESSION
#define  PAM_SM_SESSION
#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <security/pam_ext.h>
#include <sys/types.h>
#endif // PAM_SM_SESSION

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

typedef enum bool_ {
	false,true
} bool;

typedef struct locked_ressources_
{
  int fd;
  pam_handle_t *pamh;
} locked_ressources;

static void on_cancel_file_lock(void *param)
{
  if (param) {
	locked_ressources *rsc = (locked_ressources *)param;
    if (rsc->fd != -1) {
    	int error = EXIT_SUCCESS;
		if (unlikely(flock(rsc->fd,LOCK_UN) == 0)) {
			pam_handle_t *pamh = rsc->pamh;
			error = errno;
			ERROR_MSG("flock LOCK_UN error %d (%m)",error);
		}
		close(rsc->fd);
		rsc->fd = -1;
    }
  }
}

static int manage_sessions_number(pam_handle_t *pamh,const uid_t uid, const bool session_opening, bool *last_session) {
	int error = EXIT_SUCCESS;
	// created by pam_systemd and used for storing files used by running processes for that user.
#define RUN_USER_PATH "/run/user/%u/.pam_unshared_session_counter"
	size_t length = 5 + strlen(RUN_USER_PATH) + 1;
	char *run_user_path = (char *)alloca(length);
	sprintf(run_user_path,RUN_USER_PATH,uid);
	int fd = open(run_user_path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if (likely((fd != -1))) {
		if (likely(flock(fd,LOCK_EX) == 0)) {
			unsigned int current_value = 0;
			locked_ressources ressources = {fd,pamh};
			pthread_cleanup_push(on_cancel_file_lock,&ressources);
			const ssize_t n = read(fd,&current_value,sizeof(current_value));
			if (likely(sizeof(current_value) == n)) {
				if (session_opening) {
					++current_value;
				} else {
					--current_value;
				}

				if (current_value > 0) {
					const ssize_t written = write(fd,&current_value,sizeof(current_value));
					if (unlikely(written != sizeof(current_value))) {
						error = errno;
						ERROR_MSG("write %s error %d (%m)",run_user_path,error);
					}
				} else {
					*last_session = true;
					if (unlikely(unlink(run_user_path) != 0)) {
						error = errno;
						ERROR_MSG("unlink %s error %d (%m)",run_user_path,error);
					} else {
						INFO_MSG("File %s deleted",run_user_path);
					}
				}

			} else if (likely(n < 0)) {
				error = errno;
				ERROR_MSG("read %s error %d (%m)",run_user_path,error);
			} else {
				// n == 0
				if (session_opening) {
					current_value = 1;
					const ssize_t written = write(fd,&current_value,sizeof(current_value));
					if (unlikely(written != sizeof(current_value))) {
						error = errno;
						ERROR_MSG("write %s error %d (%m)",run_user_path,error);
					}
				} else {
					error = EINVAL;
					ERROR_MSG("Session counter file %s missing on close session",run_user_path);
				}
			}
			if (unlikely(flock(fd,LOCK_UN) != 0)) {
				error = errno;
				ERROR_MSG("flock %s LOCK_UN error %d (%m)",run_user_path,error);
			}
			pthread_cleanup_pop(0);
		} else {
			error = errno;
			ERROR_MSG("flock %s LOCK_EX error %d (%m)",run_user_path,error);
		}
		close(fd);
		fd = -1;
	} else {
		error = errno;
		ERROR_MSG("open %s O_RDWR error %d (%m)",run_user_path,error);
	}
	return error;
}


#endif /* _SESSIONS_COUNTER_H_ */

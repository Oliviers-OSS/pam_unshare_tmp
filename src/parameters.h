/*
 * parameters.h
 *
 *  Created on: 29 juil. 2019
 *      Author: oc
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#pragma once
#endif /* GCC 4.0.0 */

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#define DEFAULT_CONFIGURATION_FILE	TO_STRING(CONFIGDIR) "/security/pam_unshare_tmp.conf"
#define DEFAULT_SIZE			100000000
#define DEFAULT_MOUNT_OPTIONS	MS_NODEV|MS_NOATIME|MS_NOEXEC|MS_NOSUID
#define DEFAULT_DIR_MODE        01777

#define  PAM_SM_SESSION
#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <security/pam_ext.h>
#include <sys/types.h>

typedef enum bool_ {
	false,true
} bool;

typedef struct userParameters_ {
	pam_handle_t *pamh;
	const char *username;
	const char *groupname;
	size_t size;
	unsigned long mountflags;
	mode_t dirmode;
	bool unshare_ipc;
} userParameters;


int get_user_parameters(const char *configFile, userParameters *params);

#endif /* _PARAMETERS_H_ */

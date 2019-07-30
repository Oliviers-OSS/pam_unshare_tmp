dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT(pam-unshare-tmp, 1.0.0.0,Oliviers.Open.Source.Softwares@gmail.com,pam-unshare-tmp,https://github.com/Oliviers-OSS/pam_unshare_tmp.git)
LIB_PAM_VERSION=0
LIB_PAM_REVISON=1
LIB_PAM_INTERFACE_AGE=0
AC_CONFIG_SRCDIR([src/pam_unshare_tmp.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AC_CANONICAL_SYSTEM

dnl Initialize automake
AM_INIT_AUTOMAKE([dist-bzip2])

dnl Initialize Libtool
LT_INIT

dnl set library interface version number
AC_SUBST(LIB_PAM_VERSION)
AC_SUBST(LIB_PAM_REVISON)
AC_SUBST(LIB_PAM_INTERFACE_AGE)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB 

# Checks for header files.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h stddef.h sys/time.h syslog.h limits.h sys/mount.h sys/file.h security/pam_appl.h]) 

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T

# Checks for library functions.
AC_C_INLINE
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ALLOCA
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([mkdir strcasecmp strchr strtoul])  

#AC_CHECK_LIB(pthread,pthread_create)

# Checks for pkg config and needed parameters files
PKG_PROG_PKG_CONFIG

dnl parameters
AC_ARG_WITH([libsecuritydir],
    [AS_HELP_STRING([--with-libsecuritydir=DIR],
        [Directory for PAM modules, default /$(libdir)/security for install])],
    [], [with_libsecuritydir='$(libdir)/security'])
AC_SUBST([libsecuritydir], [$with_libsecuritydir])
AC_MSG_NOTICE([PAM installation path $with_libsecuritydir])

dnl debug
AC_ARG_ENABLE(debug,
        [  --enable-debug enable debug mode ],
        [case "${enableval}" in
         yes) enable_debug="yes";;
          no) enable_debug="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
         esac],
        [enable_debug="no"]
)
if test "$enable_debug" = "yes"; then
        AC_DEFINE(_DEBUG_, 1, [Defined if debug mode is enabled (it's easier to check).])        
fi

if test "$enable_debug" = "no"; then
        AC_DEFINE(NDEBUG, 1, [Defined if debug mode is disabled.])
fi

#if test "${enable_retail}" = "yes"; then
#        AC_DEFINE(_RETAIL_,1,[Defined is retail mode is enabled.])
#fi

AC_ARG_ENABLE(root-user,
        [  --disable-root-user disable pam_unshare_tmp for the root user (default is disabled)],
        [case "${enableval}" in
         yes) enable_root_user="yes";;
          no) enable_root_user="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug-console) ;;
         esac],
        [enable_root_user="no"]
)
if test "$enable_root_user" = "yes"; then
        AC_DEFINE(_ROOT_USER_ENABLED_, 1, [Defined if pam_unshare_tmp is enabled for the root user.])        
fi

AC_ARG_ENABLE(rpath,
        [  --enable-rpath add run-time search path hard-coded in generated executable 
        file (must be disabled when the prefix path set will be different at 
        runtime, when building rpm for example [default=yes] ], 
        [case "${enableval}" in
         yes) enable_rpath="yes";;
          no) enable_rpath="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-rpath) ;;
         esac],
        [enable_rpath="yes"]
)
AM_CONDITIONAL(RPATH_ENABLED,[test "$enable_rpath" = "yes"])

AC_ARG_ENABLE(lto,
        [  --enable-lto enable Link Time Optimization ],
        [case "${enableval}" in
         yes) enable_lto="yes";;
          no) enable_lto="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --enable-lto) ;;
         esac],
        [enable_lto="no"]
)
AM_CONDITIONAL(ENABLE_LTO,[test "$enable_lto" = "yes"])

# Documentation
#AC_CHECK_PROGS([DOXYGEN], [doxygen])
#AC_CHECK_PROGS([DOT], [dot])
#AC_CHECK_PROGS([MSCGEN], [mscgen])
#if test -z "$DOXYGEN";
#   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
#fi
              
# RPM
AC_ARG_WITH([rpmbuild-dir],
  [AS_HELP_STRING([--with-rpmbuild-dir],[location of the rpmbuild directory (optional, default $HOME/rpmbuild)])],
  [RPMBUILD_DIR="$withval"],
  [RPMBUILD_DIR="$HOME/rpmbuild"])
AC_SUBST(RPMBUILD_DIR)

AC_ARG_ENABLE(rpm-mode,
	[  --enable-rpm-mode rpm build special mode],
        [case "${enableval}" in
         yes) rpm_mode="yes";;
          no) rpm_mode="no" ;;
           *) AC_MSG_ERROR(bad value ${enableval} for --rpm_mode) ;;
         esac],
        [rpm_mode="no"]
)
AM_CONDITIONAL([RPM_MODE],[test "$rpm_mode" = "yes"])

AC_ARG_WITH([rpmbuild-pkgconfig-path],
  [AS_HELP_STRING([--with-rpmbuild-pkgconfig-path],[pkgconfig-path to use during rpmbuild directory (optional, default $PKG_CONFIG_PATH if set)])],
  [RPMBUILD_PKG_CONFIG_PATH="$withval"],
  [RPMBUILD_PKG_CONFIG_PATH="$PKG_CONFIG_PATH"])
AC_SUBST(RPMBUILD_PKG_CONFIG_PATH)

AC_SUBST(SCM_LABEL,[$(git describe --exact-match --tags $(git log -n1 --pretty='%n') 2>/dev/null)])
	
AC_CONFIG_FILES(Makefile 	
	src/Makefile
	)
AC_OUTPUT


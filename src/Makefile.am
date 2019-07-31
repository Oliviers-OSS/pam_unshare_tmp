AUTOMAKE_OPTIONS = foreign

#######################################
# The list of libraries we are building seperated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
libsecurity_LTLIBRARIES = pam_unshare_tmp.la


#######################################
# Build information for each library

# Sources for pam_unshare_tmp
noinst_HEADERS = debug.h ModuleVersionInfo.h parameters.h ini.h
pam_unshare_tmp_la_SOURCES = pam_unshare_tmp.c
pam_unshare_tmp_la_SOURCES += parameters.c ini.c  
pam_unshare_tmp_la_SOURCES += $(noinst_HEADERS) pam_unshare_tmp.map
pam_unshare_tmp_la_SOURCES += version.c


# Compiler options. Here we are adding the include directory
# to be searched for headers included in the source code.
pam_unshare_tmp_la_CPPFLAGS = -I$(top_srcdir)  
pam_unshare_tmp_la_CPPFLAGS +=-DCONFIGDIR="$(sysconfdir)"
pam_unshare_tmp_la_CPPFLAGS +=-D_SCM_LABEL="$(SCM_LABEL)"
pam_unshare_tmp_la_CFLAGS =  -pipe -Wall -Wextra -Wformat
pam_unshare_tmp_la_CFLAGS += -Wmissing-include-dirs -Wmissing-noreturn  -Wpacked  -Wparentheses
pam_unshare_tmp_la_CFLAGS += -Wconversion -Winline 
#pam_unshare_tmp_la_CFLAGS += -pedantic  
pam_unshare_tmp_la_CFLAGS += -Wpointer-arith -Wformat-security -Wformat-nonliteral -Winit-self -Wwrite-strings
pam_unshare_tmp_la_CFLAGS += -Wmissing-braces  -Wmissing-field-initializers -Wmissing-format-attribute
pam_unshare_tmp_la_CFLAGS += -fstack-protector-strong -Wl,-z,relro -Wl,-z,now

if ENABLE_LTO
pam_unshare_tmp_la_CFLAGS += -flto
endif

# Linker options 
pam_unshare_tmp_la_LDFLAGS =  -lpam -module -lpthread
pam_unshare_tmp_la_LDFLAGS += -version-info ${LIB_PAM_VERSION}:${LIB_PAM_REVISON}:${LIB_PAM_INTERFACE_AGE}
pam_unshare_tmp_la_LDFLAGS += -Wl,--version-script=$(srcdir)/pam_unshare_tmp.map
pam_unshare_tmp_la_LDFLAGS += -Wl,--build-id=sha1 -rdynamic
pam_unshare_tmp_la_LDFLAGS += -Wl,-e,displayVersionInfo


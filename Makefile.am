AUTOMAKE_OPTIONS = foreign
SUBDIRS=src 
ACLOCAL_AMFLAGS=-I m4

dist-rpm: distdir
	tardir=$(distdir) && $(am__tar) | GZIP=$(GZIP_ENV) gzip -c >$(distdir).tar.gz
	$(am__post_remove_distdir)
	cp -vf $(distdir).tar.gz $(RPMBUILD_DIR)/SOURCES/	
	rpmbuild -ba pam-unshare-tmp.spec
	
cppcheck:
	cppcheck --enable=all $(top_srcdir)
	
count:
	sloccount $(top_srcdir)/src 
	

bin_PROGRAMS = leas
leas_SOURCES = src/leas.c
leasdir = $(datadir)
leas_DATA = src/leas.scm
leas_CFLAGS = $(GUILE_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
leas_LDFLAGS = $(GUILE_LDFLAGS)
man1_MANS = doc/leas.1

check_PROGRAMS=test_util
test_util_SOURCES=test/test_util.c
test_util_CFLAGS = $(GUILE_CFLAGS) -DLEAS_SCM_INSTALL='"$(datadir)/leas.scm"'
test_util_LDFLAGS = $(GUILE_LDFLAGS)
TESTS=test_util

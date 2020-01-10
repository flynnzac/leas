bin_PROGRAMS = bal
bal_SOURCES = src/bal.c
baldir = $(datadir)
bal_DATA = src/bal.scm
bal_CFLAGS = $(GUILE_CFLAGS) -DBAL_SCM_INSTALL='"$(datadir)/bal.scm"'
bal_LDFLAGS = $(GUILE_LDFLAGS)


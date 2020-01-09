ifndef BAL_SCM_INSTALL
	BAL_SCM_INSTALL=/etc/bal.scm
endif
ifndef PREFIX
	PREFIX=/usr/local
endif
ifndef GUILE_CFLAGS
	GUILE_CFLAGS=`pkg-config --libs --cflags guile-2.2`
endif


bal: bal.c 
	cc  $(GUILE_CFLAGS) bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall -DBAL_SCM_INSTALL=\"$(BAL_SCM_INSTALL)\" -lm -g

.FORCE:

doc: doc/bal.1 doc/bal.texinfo .FORCE
	(groff -mandoc -Tascii doc/bal.1 | col -b) | fold -s - > README
	guile postdoc.scm
	makeinfo doc/bal.texinfo -o doc/bal.info
	makeinfo --pdf doc/bal.texinfo -o doc/bal.pdf

install: bal doc/bal.1 doc/bal.texinfo bal.scm
	mkdir -p $(PREFIX)/share/man/man1/
	cp doc/bal.1 $(PREFIX)/share/man/man1/
	cp bal.scm $(BAL_SCM_INSTALL)
	guild compile $(BAL_SCM_INSTALL)
	cp bal $(PREFIX)/bin/




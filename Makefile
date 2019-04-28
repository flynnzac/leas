bal: bal.c 
	cc  -I /usr/include/guile/2.2 bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall


doc: doc/bal.1 doc/bal.mro.texinfo
	(groff -mandoc -Tascii doc/bal.1 | col -b) | fold -s - > README
	guile postdoc.scm
	cat doc/bal.mro.texinfo | mro > doc/bal.texinfo
	makeinfo bal.texinfo
	makeinfo --pdf bal.texinfo

install: bal bal.1 bal.scm
	mkdir -p /usr/local/share/man/man1/
	cp bal.1 /usr/local/share/man/man1/
	cp bal.scm /etc/
	guild compile /etc/bal.scm
	cp bal /usr/local/bin/


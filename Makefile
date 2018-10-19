bal: bal.c 
	cc  -I /usr/include/guile/2.2 bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall

doc: bal.1 ABOUT
	(echo "Man Page\n---\n" && man -P cat -l bal.1 && echo "\n---\n" && cat ABOUT) | fold -s - > README
	groff -ms -e baldoc.ms > baldoc.ps

install: bal bal.1
	mkdir -p /usr/local/share/man/man1/
	cp bal.1 /usr/local/share/man/man1/
	cp bal /usr/local/bin/

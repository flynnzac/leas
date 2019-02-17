bal: bal.c 
	cc  -I /usr/include/guile/2.2 bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall

doc: bal.1 
	(echo "bal's website: http://zflynn.com/bal\nMan Page\n---\n" && (groff -mandoc -Tascii bal.1 | col -b)) | fold -s - > README
	groff -mandoc -Thtml bal.1 > docs/bal.html
	guile postdoc.scm

install: bal bal.1
	mkdir -p /usr/local/share/man/man1/
	cp bal.1 /usr/local/share/man/man1/
	cp bal /usr/local/bin/

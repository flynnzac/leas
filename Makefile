bal: bal.c 
	cc  -I /usr/include/guile/2.2 bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall

doc: bal.1 ABOUT
	(echo "bal's website: http://zflynn.com/bal\nMan Page\n---\n" && man -P cat -l bal.1 && echo "\n---\n" && cat ABOUT) | fold -s - > README
	groff -mspdf -Tpdf -e baldoc.ms > docs/baldoc.pdf
	groff -ms -Thtml -e baldoc.ms > docs/baldoc.html
	groff -Thtml -man bal.1 > docs/bal.html
	guile postdoc.scm

install: bal bal.1
	mkdir -p /usr/local/share/man/man1/
	cp bal.1 /usr/local/share/man/man1/
	cp bal /usr/local/bin/

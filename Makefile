bal: bal.c .FORCE
	cc  -I /usr/include/guile/2.2 bal.c -lguile-2.2 -lgc -lcsv -lreadline -o bal -Wall

.FORCE:

doc: bal.1 ABOUT
	echo "Man Page\n---\n" > README
	man -P cat -l bal.1 >> README
	echo "\n---\n" >> README
	cat ABOUT >> README

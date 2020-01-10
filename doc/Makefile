doc: bal.1 bal.texinfo 
	(groff -mandoc -Tascii bal.1 | col -b) | fold -s - > ../README
	guile postdoc.scm
	makeinfo bal.texinfo -o bal.info
	makeinfo --pdf bal.texinfo -o bal.pdf

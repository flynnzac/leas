doc: leas.1 leas.texinfo 
	(groff -mandoc -Tascii leas.1 | col -b) | fold -s - > ../README
	guile postdoc.scm
	cp ../README.md ../README
	makeinfo leas.texinfo -o ../docs/leas.info
	makeinfo --pdf leas.texinfo -o ../docs/leas.pdf
	makeinfo --html leas.texinfo -o ../docs


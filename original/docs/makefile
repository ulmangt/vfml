all:
	doxygen doxygen.config
	cp -r . /homes/gws/ghulten/www/vfml
	cp -r . /projects/dm/www/vfml

refman: 
	doxygen doxygen.config
	(cd latex ; make)
	cp latex/refman.ps downloads/vfml-reference.ps
	cp latex/refman.pdf downloads/vfml-reference.pdf
	cp -r . /homes/gws/ghulten/www/vfml
	cp -r . /projects/dm/www/vfml

clean:
	rm -f *.html
	rm *.png
	rm latex/*

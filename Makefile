

all: scopeserv scopev scopecmd

obj:
	mkdir -p obj

scopeserv:
	(make -f Makefile.scopeserv)

scopecmd:
	(make -f Makefile.scopecmd)

scopev:
	(make -f Makefile.scopev)

clean_all: clean
	rm scopeserv scopev scopecmd

clean:
	rm -rf obj/*.o


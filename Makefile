# by: MaG.

include Makefile.inc

DIRS = src lib/http-parser

all: linux-http-proxy

linux-http-proxy: lib
	$(ECHO) looking into src: $(MAKE) $(MFLAGS)
	cd src; $(MAKE) $(MFLAGS)

lib: force_look
	$(ECHO) looking into lib/: $(MAKE) $(MFLAGS)
	cd lib; $(MAKE) $(MFLAGS)

clean:
	$(RM) -f $(NAME)
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look:
	true


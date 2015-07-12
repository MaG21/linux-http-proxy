# by: MaG.

include Makefile.inc

DIRS = src lib/http-parser

all: linux-http-proxy

linux-http-proxy: http-parser
	$(ECHO) looking into src: $(MAKE) $(MFLAGS)
	cd src; $(MAKE) $(MFLAGS)

http-parser: force_look
	$(ECHO) looking into lib/http-parser: $(MAKE) $(MFLAGS)
	cd lib/http-parser; $(MAKE) $(MFLAGS)

clean:
	$(RM) -f $(NAME)
	for d in $(DIRS); do (cd $$d; $(MAKE) clean ); done

force_look:
	true


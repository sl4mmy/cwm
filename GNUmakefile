BINDIR = /usr/bin
MANDIR = /usr/share/man

CFLAGS = -O2 -I /usr/include/freetype2 -D_GNU_SOURCE
LDFLAGS =
LIBS = -lbsd -lXft -lXrender -lX11 -lxcb -lXau -lXdmcp -lfontconfig -lexpat -lfreetype -lz -lXinerama -lXrandr -lXext
YACC = yacc

cwm: calmwm.o client.o conf.o group.o kbfunc.o menu.o mousefunc.o parse.o pledge.o reallocarray.o screen.o search.o util.o xevents.o xmalloc.o xutil.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f cwm *.o parse.c y.tab.h

install: cwm
	install -o root -g bin -m 0555 cwm $(BINDIR)
	install -o root -g root -m 0444 cwm.1 $(MANDIR)/man1
	install -o root -g root -m 0444 cwmrc.5 $(MANDIR)/man5

uninstall:
	rm -f $(BINDIR)/cwm $(MANDIR)/man1/cwm.1 $(MANDIR)/man5/cwmrc.5

calmwm.h: openbsd_compat.h

*.c: calmwm.h

parse.c: parse.y calmwm.h
	$(YACC) -d $<
	mv y.tab.c $@

.PHONY: clean install uninstall

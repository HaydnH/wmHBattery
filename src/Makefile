CC     = gcc
CFLAGS = -O2 -Wall
INCDIR = -I/usr/X11R6/include/X11 -I/usr/X11R6/include -I/usr/include/cairo
DESTDIR= /usr
MANDIR = /usr/share/man
LIBDIR = -L/usr/X11R6/lib

# for Linux
LIBS   = -lXpm -lX11 -lXext -lm -lcairo

# for Solaris
# LIBS   = -lXpm -lX11 -lXext -lsocket -lnsl

OBJS   = wmHBattery.o \
         xutils_cairo.o


.c.o:
	$(CC) $(CFLAGS) -D$(shell echo `uname -s`) -c $< -o $*.o $(INCDIR)


all:	wmHBattery.o wmHBattery

wmHBattery.o: wmHBattery_master.xpm wmHBattery_mask.xbm
wmHBattery:	$(OBJS) 
	$(CC) $(COPTS) $(SYSTEM) -o wmHBattery $^ $(INCDIR) $(LIBDIR) $(LIBS)

clean:
	for i in $(OBJS) ; do \
		rm -f $$i; \
	done
	rm -f wmHBattery

install:: wmHBattery
	install -c -s -m 0755 wmHBattery $(DESTDIR)/bin
	install -c -m 0644 wmHBattery.1 $(MANDIR)/man1 


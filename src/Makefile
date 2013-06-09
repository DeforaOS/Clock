TARGETS	= clock
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem gtk+-2.0`
CFLAGS	= -Wall -g -O2
LDFLAGS	= `pkg-config --libs libSystem gtk+-2.0`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

clock_OBJS = clock.o main.o
clock_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
clock_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

clock: $(clock_OBJS)
	$(CC) -o clock $(clock_OBJS) $(clock_LDFLAGS)

clock.o: clock.c clock.h
	$(CC) $(clock_CFLAGS) -c clock.c

main.o: main.c clock.h ../config.h
	$(CC) $(clock_CFLAGS) -c main.c

clean:
	$(RM) -- $(clock_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 clock $(DESTDIR)$(BINDIR)/clock

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/clock

.PHONY: all clean distclean install uninstall

TARGETS	= $(OBJDIR)clock
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem gtk+-2.0`
CFLAGS	= -Wall -g -O2
LDFLAGS	= `pkg-config --libs libSystem gtk+-2.0` -lintl
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

clock_OBJS = $(OBJDIR)clock.o $(OBJDIR)main.o
clock_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
clock_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)clock: $(clock_OBJS)
	$(CC) -o $(OBJDIR)clock $(clock_OBJS) $(clock_LDFLAGS)

$(OBJDIR)clock.o: clock.c clock.h
	$(CC) $(clock_CFLAGS) -o $(OBJDIR)clock.o -c clock.c

$(OBJDIR)main.o: main.c clock.h ../config.h
	$(CC) $(clock_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	$(RM) -- $(clock_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)clock $(DESTDIR)$(BINDIR)/clock

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/clock

.PHONY: all clean distclean install uninstall

# BSD conventions by default, override to taste
prefix ?= /usr/local
bindir ?= $(prefix)/bin

CFLAGS += -ansi -pthread -g
CFLAGS += -Wall -Wextra -pedantic

LDFLAGS += -pthread

all: netflood

clean:
	rm -f netflood

install: netflood
	install -d $(bindir)
	install netflood $(bindir)/

uninstall:
	rm -f $(bindir)/netflood

.PHONY: all install uninstall

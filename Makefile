CFLAGS += -Wall -Wextra -pthread
LDLIBS += -pthread
PREFIX ?= /usr/local

all: netflood

clean:
	rm -f netflood

install: netflood
	install -d $(PREFIX)/bin \
	           $(PREFIX)/share/doc/netflood
	install netflood   $(PREFIX)/bin/
	install LICENSE.md $(PREFIX)/share/doc/netflood/

uninstall:
	rm -f $(PREFIX)/bin/netflood \
	      $(PREFIX)/share/doc/netflood/LICENSE.md
	-rmdir $(PREFIX)/share/doc/netflood

.PHONY: all install uninstall

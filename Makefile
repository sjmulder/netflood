CFLAGS += -Wall -Wextra -pthread
LDLIBS += -pthread

all: netflood

clean:
	rm -f netflood

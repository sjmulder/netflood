#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define MIN(a,b) ((a)>(b)?(b):(a))

static void
readall(int fd, char **bufp, size_t *lenp)
{
	size_t	 cap	= 4096;
	ssize_t	 nread;

	*lenp = 0;
	*bufp = malloc(cap);

	if (!*bufp)
		err(1, NULL);
	
	
	while (1) {
		nread = read(fd, *bufp+*lenp, cap-*lenp);
		if (nread == 0)
			return;
		if (nread == -1)
			err(1, NULL);

		*lenp += nread;
		if (*lenp == cap) {
			cap *= 2;
			*bufp = realloc(*bufp, cap);
			if (!*bufp)
				err(1, NULL);
		}
	}
}

static void
writeall(int fd, char *data, size_t len)
{
	ssize_t	nwritten;

	while (len > 0) {
		nwritten = write(fd, data, MIN(len, 8));
		if (nwritten == -1)
			err(1, NULL);

		data += nwritten;
		len -= nwritten;
	}
}

static void
forward(int fdin, int fdout)
{
	ssize_t	nread;
	char	buf[4096];

	while (1) {
		nread = read(fdin, buf, sizeof(buf));
		if (nread == -1)
			err(1, NULL);
		if (nread == 0)
			break;

		writeall(fdout, buf, nread);
	}
}

int
main(int argc, char **argv)
{
	char		*input;
	size_t		 inputlen;
	struct addrinfo	 hints;
	struct addrinfo	*addr0, *addr;
	int		 error;
	int		 sock	= -1;

	if (argc != 3)
		errx(1, "usage: netflood [host] [port | service]");

	readall(STDIN_FILENO, &input, &inputlen);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = IPPROTO_TCP;

	error = getaddrinfo(argv[1], argv[2], &hints, &addr0);
	if (error)
		errx(1, "lookup failed: %s", gai_strerror(error));

	for (addr = addr0; addr; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype,
		    addr->ai_protocol);
		if (sock == -1) {
			warn("socket()");
			continue;
		}

		if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1) {
			warn("connect()");
			close(sock);
			sock = -1;
			continue;
		}

		break;
	}

	if (sock == -1)
		errx(1, "failed to connect");

	writeall(sock, input, inputlen);
	forward(sock, STDOUT_FILENO);

	close(sock);
	freeaddrinfo(addr0);
	return 0;
}

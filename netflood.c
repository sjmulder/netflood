/* netflood.c - Copyright (c) 2018, Sijmen J. Mulder (see LICENSE.md) */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <pthread.h>

struct threadcx {
	struct addrinfo	*addr0;
	char		*data;
	size_t		 datalen;
};

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
		nwritten = write(fd, data, len);
		if (nwritten == -1)
			err(1, NULL);

		data += nwritten;
		len -= nwritten;
	}
}

static void
drain(int fd)
{
	char	data[4096];
	ssize_t	nread;

	while (1) {
		nread = read(fd, data, sizeof(data));
		if (nread == -1)
			err(1, NULL);
		if (nread == 0)
			return;
	}
}

static int
connectany(struct addrinfo *addr0)
{
	struct addrinfo	*addr;
	int		 sock = -1;

	for (addr = addr0; addr; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype,
		    addr->ai_protocol);
		if (sock == -1)
			continue;

		if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1) {
			close(sock);
			continue;
		}

		return sock;
	}

	err(1, "failed to connect");
}

static void *
threadmain(void *p)
{
	struct threadcx	*cx	= p;
	int		 sock;

	while (1) {
		sock = connectany(cx->addr0);
		writeall(sock, cx->data, cx->datalen);
		drain(sock);
		close(sock);
		write(STDOUT_FILENO, ".", 1);
	}

	return NULL;
}

int
main(int argc, char **argv)
{
	int		 opt;
	long		 njobs	= 1;
	struct threadcx	 threadcx;
	struct addrinfo	 hints;
	int		 error;
	int		 i;
	pthread_t	*threads;

	while ((opt = getopt(argc, argv, "j:")) != -1) {
		switch (opt) {
		case 'j':
			njobs = strtol(optarg, NULL, 10);
			if (njobs < 1)
				errx(1, "invalid -j value");
			break;
		default:
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		errx(1, "usage: netflood [-j njobs] host port|service");

	readall(STDIN_FILENO, &threadcx.data, &threadcx.datalen);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = IPPROTO_TCP;

	error = getaddrinfo(argv[0], argv[1], &hints, &threadcx.addr0);
	if (error)
		errx(1, "lookup failed: %s", gai_strerror(error));

	threads = malloc(njobs * sizeof(*threads));
	if (!threads)
		err(1, NULL);

	for (i = 0; i < njobs; i++)
		pthread_create(&threads[i], NULL, threadmain, &threadcx);
	for (i = 0; i < njobs; i++)
		pthread_join(threads[i], NULL);

	freeaddrinfo(threadcx.addr0);
	return 0;
}

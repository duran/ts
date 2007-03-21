#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <stdio.h>

#include "msg.h"

const char path[] = "/tmp/prova.socket";

enum
{
	MAXCONN=100
};

enum Break
{
	BREAK,
	NOBREAK
};

void server_loop(int ls);
enum Break
client_read(int index, int *connections, int *nconnections);
void end_server(int ls);

void server_main()
{
	int ls,cs;
	struct sockaddr_un addr;
	int res;

	ls = socket(PF_UNIX, SOCK_STREAM, 0);
	assert(ls != -1);

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);

	res = bind(ls, (struct sockaddr *) &addr, sizeof(addr));
	if (res == -1)
	{
		perror("Error binding.");
		return;
	}

	res = listen(ls, 0);
	if (res == -1)
	{
		perror("Error listening.");
		return;
	}

	printf("server loop\n");
	server_loop(ls);
}

void server_loop(int ls)
{
	fd_set readset;
	int connections[MAXCONN];
	int nconnections;
	int i;
	int maxfd;
	int keep_loop = 1;

	while (keep_loop)
	{
		FD_ZERO(&readset);
		FD_SET(ls,&readset);
		maxfd = ls;
		for(i=0; i< nconnections; ++i)
		{
			FD_SET(connections[i], &readset);
			if (connections[i] > maxfd)
				maxfd = connections[i];
		}
		printf("select: nc = %i\n", nconnections);
		select(maxfd + 1, &readset, NULL, NULL, NULL);
		printf("Select unblocks\n");
		if (FD_ISSET(ls,&readset))
		{
			int cs;
			cs = accept(ls, NULL, NULL);
			assert(cs != -1);
			connections[nconnections++] = cs;
		}
		for(i=0; i< nconnections; ++i)
			if (FD_ISSET(connections[i], &readset))
			{
				enum Break b;
				b = client_read(i, connections,
						&nconnections);
				/* Check if we should break */
				if (b == BREAK)
					keep_loop = 0;
			}
	}

	end_server(ls);
}

void end_server(int ls)
{
	close(ls);
	unlink(path);
}

void remove_connection(int index, int *connections, int *nconnections)
{
	int i;
	for(i=index; i<(*nconnections-1); ++i)
	{
		connections[i] = connections[i+1];
	}
	(*nconnections)--;
}

enum Break
client_read(int index, int *connections, int *nconnections)
{
	struct msg data;
	int s;
	int res;

	s = connections[index];

	/* Read the message */
	res = read(s, &data, sizeof(data));
	assert(res != -1);
	if (res == 0)
	{
		close(s);
		remove_connection(index, connections, nconnections);
		return NOBREAK;
	}

	printf("recv. data.type = %i\n", data.type);

	/* Process message */
	if (data.type == KILL)
		return BREAK; /* break in the parent*/

	return NOBREAK; /* normal */
}

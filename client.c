#include <assert.h>
#include "msg.h"

int shutdown_server()
{
	struct msg m;
	int res;

	m.type = KILL;
	res = write(server_socket, &m, sizeof(m));
	assert(res != -1);
}

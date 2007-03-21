#include <assert.h>
#include "msg.h"
#include "main.h"

void c_new_job(const char *command)
{
    struct msg m;
    int res;

    m.type = NEWJOB;

    strcpy(m.u.command, command);

    res = write(server_socket, &m, sizeof(m));
    if(res == -1)
        perror("write");
}

void c_wait_server_commands()
{
    struct msg m;
    int res;

    while (1)
    {
        res = read(server_socket, &m, sizeof(m));
        if(res == -1)
            perror("read");

        if (res == 0)
            break;
        assert(res == sizeof(m));
        msgdump(&m);
        if (m.type == NEWJOB_OK)
            ;
        if (m.type == LIST_LINE)
        {
            printf("%s", m.u.line);
        }
    }
}

void c_wait_server_lines()
{
    struct msg m;
    int res;

    while (1)
    {
        res = read(server_socket, &m, sizeof(m));
        if(res == -1)
            perror("read");

        if (res == 0)
            break;
        assert(res == sizeof(m));
        msgdump(&m);
        if (m.type == LIST_LINE)
        {
            printf("%s", m.u.line);
        }
    }
}

void c_list_jobs(const char *command)
{
    struct msg m;
    int res;

    m.type = LIST;

    res = write(server_socket, &m, sizeof(m));
    if(res == -1)
        perror("write");
}

int c_shutdown_server()
{
    struct msg m;
    int res;

    m.type = KILL;
    res = write(server_socket, &m, sizeof(m));
    assert(res != -1);
}

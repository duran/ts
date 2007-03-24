#include <unistd.h>
#include <assert.h>

#include <stdio.h>

#include "main.h"

extern char *optarg;
extern int optind, opterr, optopt;

int kill_server = 0;
int need_server = 0;
int list_jobs = 0;

int server_socket;

/* !!! We should control buffer overflow */
char new_command[500];

void parse_opts(int argc, char **argv)
{
    int c;

    /* Parse options */
    while(1) {
        c = getopt(argc, argv, "kl");

        if (c == -1)
            break;

        switch(c)
        {
            case 'k':
                kill_server = 1;
                break;
            case 'l':
                list_jobs = 1;
                break;
        }
    }

    new_command[0] = '\0';
    /* !!! We should control buffer overflow */
    if (optind < argc)
    {
        strcpy(new_command, argv[optind]);
        ++optind;
        while (optind < argc)
        {
            strcat(new_command, " ");
            strcat(new_command, argv[optind++]);
        }
    }

    if (list_jobs || kill_server || (new_command[0] != '\0'))
        need_server = 1;
}

static int go_background()
{
    int pid;
    pid = fork();

    switch(pid)
    {
        case -1:
            perror("fork failed");
            exit(-1);
            break;
        case 0:
            break;
        default:
            exit(0);
    }
}

int main(int argc, char **argv)
{
    parse_opts(argc, argv);

    if (need_server)
        ensure_server_up();

    if (new_command[0] != '\0')
    {
        go_background();
        assert(need_server);
        c_new_job(new_command);
        c_wait_server_commands();
    }

    if (list_jobs != 0)
    {
        assert(need_server);
        c_list_jobs();
        c_wait_server_lines();
    }
    
    if (kill_server)
    {
        assert(need_server);
        c_shutdown_server();
    }

    if (need_server)
    {
        close(server_socket);
    }

    return 0;
}

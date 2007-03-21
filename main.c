#include <unistd.h>

#include <stdio.h>

extern char *optarg;
extern int optind, opterr, optopt;

int kill_server = 0;
int need_server = 0;

int server_socket;

void parse_opts(int argc, char **argv)
{
    int c;

    /* Parse options */
    while(1) {
        c = getopt(argc, argv, "k");

        if (c == -1)
            break;

        switch(c)
        {
            case 'k':
                kill_server = 1;
                break;
        }
    }

    if (kill_server == 0)
        need_server = 1;
}

int main(int argc, char **argv)
{
    parse_opts(argc, argv);

    need_server = 1;

    if (need_server)
    {
        fprintf(stderr, "Ensure server up.\n");
        ensure_server_up();
    }
    
    if (kill_server)
    {
        printf("Trying to kill server.\n");
        shutdown_server();
    }

    return 0;
}

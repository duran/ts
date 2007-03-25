#include <unistd.h>
#include <assert.h>

#include <stdio.h>

#include "main.h"

extern char *optarg;
extern int optind, opterr, optopt;

int kill_server = 0;
int need_server = 0;
int clear_finished = 0;
int list_jobs = 0;
int store_output = 1;
int should_go_background = 1;

int server_socket;

/* Allocated in get_command() */
char *new_command;

void get_command(int index, int argc, char **argv)
{
    int size;
    int i;
    
    size = 0;
    /* Count bytes needed */
    for (i = index; i < argc; ++i)
    {
        /* The '1' is for spaces, and at the last i,
         * for the null character */
        size = size + strlen(argv[i]) + 1;
    }

    /* Alloc */
    new_command = malloc(size);
    assert(new_command != NULL);

    /* Build the command */
    strcpy(new_command, argv[index]);
    for (i = index+1; i < argc; ++i)
    {
        strcat(new_command, " ");
        strcat(new_command, argv[i]);
    }
}

void parse_opts(int argc, char **argv)
{
    int c;

    /* Parse options */
    while(1) {
        c = getopt(argc, argv, "klcnB");

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
            case 'c':
                clear_finished = 1;
                break;
            case 'n':
                store_output = 0;
                break;
            case 'B':
                should_go_background = 0;
                break;
        }
    }

    new_command = 0;

    if (optind < argc)
        get_command(optind, argc, argv);

    if (list_jobs || kill_server || (new_command != 0)
            || clear_finished)
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

    if (new_command != 0)
    {
        if (should_go_background)
            go_background();
        assert(need_server);
        c_new_job(new_command, store_output);
        c_wait_server_commands(new_command, store_output);
        free(new_command);
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

    if (clear_finished)
    {
        assert(need_server);
        c_clear_finished();
    }

    if (need_server)
    {
        close(server_socket);
    }

    return 0;
}

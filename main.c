#include <unistd.h>
#include <assert.h>

#include <stdio.h>

#include "main.h"

extern char *optarg;
extern int optind, opterr, optopt;

/* Globals */
struct Command_line command_line;
int server_socket;

/* Allocated in get_command() */
char *new_command;


static void default_command_line()
{
    command_line.request = c_SHOW_HELP;
    command_line.need_server = 0;
    command_line.store_output = 1;
    command_line.should_go_background = 1;
}

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
        c = getopt(argc, argv, ":+Klcnft:");

        if (c == -1)
            break;

        switch(c)
        {
            case 'K':
                command_line.request = c_KILL_SERVER;
                break;
            case 'l':
                command_line.request = c_LIST;
                break;
            case 'c':
                command_line.request = c_CLEAR_FINISHED;
                break;
            case 'n':
                command_line.store_output = 0;
                break;
            case 'f':
                command_line.should_go_background = 0;
                break;
            case 't':
                command_line.request = c_TAIL;
                command_line.jobid = atoi(optarg);
                break;
            case ':':
                switch(optopt)
                {
                    case 't':
                        command_line.request = c_TAIL;
                        command_line.jobid = -1; /* This means the 'last' job */
                        break;
                    default:
                        fprintf(stderr, "Option %c missing argument: %s\n",
                                optopt, optarg);
                        exit(-1);
                }
                break;
            case '?':
                fprintf(stderr, "Wrong option %c.\n", optopt);
                exit(-1);
        }
    }

    new_command = 0;

    if (optind < argc && command_line.request == c_SHOW_HELP)
    {
        command_line.request = c_QUEUE;
        get_command(optind, argc, argv);
    }

    if (command_line.request != c_SHOW_HELP)
        command_line.need_server = 1;
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
    default_command_line();
    parse_opts(argc, argv);

    if (command_line.need_server)
        ensure_server_up();

    switch(command_line.request)
    {
    case c_QUEUE:
        assert(new_command != 0);
        if (command_line.should_go_background)
            go_background();
        assert(command_line.need_server);
        c_new_job(new_command);
        c_wait_server_commands(new_command);
        free(new_command);
        break;
    case c_LIST:
        assert(command_line.need_server);
        c_list_jobs();
        c_wait_server_lines();
        break;
    case c_KILL_SERVER:
        assert(command_line.need_server);
        c_shutdown_server();
        break;
    case c_CLEAR_FINISHED:
        assert(command_line.need_server);
        c_clear_finished();
        break;
    case c_TAIL:
        assert(command_line.need_server);
        c_tail();
        /* This will not return! */
        break;
    }

    if (command_line.need_server)
    {
        close(server_socket);
    }

    return 0;
}

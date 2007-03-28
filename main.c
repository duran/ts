/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "main.h"

extern char *optarg;
extern int optind, opterr, optopt;

/* Globals */
struct Command_line command_line;
int server_socket;

/* Allocated in get_command() */
char *new_command;

static char version[] = "Task Spooler v0.2.3 - a task queue system for the unix user.\n"
"Copyright (C) 2007  Lluis Batlle i Rossell";

static void default_command_line()
{
    command_line.request = c_LIST;
    command_line.need_server = 0;
    command_line.store_output = 1;
    command_line.should_go_background = 1;
    command_line.should_keep_finished = 1;
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
    new_command = (char *) malloc(size);
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
        c = getopt(argc, argv, ":VhKClnfr:t:c:o:p:w:u:");

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
            case 'h':
                command_line.request = c_SHOW_HELP;
                break;
            case 'V':
                command_line.request = c_SHOW_VERSION;
                break;
            case 'C':
                command_line.request = c_CLEAR_FINISHED;
                break;
            case 'c':
                command_line.request = c_CAT;
                command_line.jobid = atoi(optarg);
                break;
            case 'o':
                command_line.request = c_SHOW_OUTPUT_FILE;
                command_line.jobid = atoi(optarg);
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
            case 'p':
                command_line.request = c_SHOW_PID;
                command_line.jobid = atoi(optarg);
                break;
            case 'r':
                command_line.request = c_REMOVEJOB;
                command_line.jobid = atoi(optarg);
                break;
            case 'w':
                command_line.request = c_WAITJOB;
                command_line.jobid = atoi(optarg);
                break;
            case 'u':
                command_line.request = c_URGENT;
                command_line.jobid = atoi(optarg);
                break;
            case ':':
                switch(optopt)
                {
                    case 't':
                        command_line.request = c_TAIL;
                        command_line.jobid = -1; /* This means the 'last' job */
                        break;
                    case 'c':
                        command_line.request = c_CAT;
                        command_line.jobid = -1; /* This means the 'last' job */
                        break;
                    case 'o':
                        command_line.request = c_SHOW_OUTPUT_FILE;
                        command_line.jobid = -1; /* This means the 'last' job */
                        break;
                    case 'p':
                        command_line.request = c_SHOW_PID;
                        command_line.jobid = -1; /* This means the 'last' job */
                        break;
                    case 'r':
                        command_line.request = c_REMOVEJOB;
                        command_line.jobid = -1; /* This means the 'last'
                                                    added job */
                        break;
                    case 'w':
                        command_line.request = c_WAITJOB;
                        command_line.jobid = -1; /* This means the 'last'
                                                    added job */
                        break;
                    case 'u':
                        command_line.request = c_URGENT;
                        command_line.jobid = -1; /* This means the 'last'
                                                    added job */
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

    /* if the request is still the default option... 
     * (the default values should be centralized) */
    if (optind < argc && command_line.request == c_LIST)
    {
        command_line.request = c_QUEUE;
        get_command(optind, argc, argv);
    }

    if (command_line.request != c_SHOW_HELP &&
            command_line.request != c_SHOW_VERSION)
        command_line.need_server = 1;

    if ( ! command_line.store_output && ! command_line.should_go_background )
        command_line.should_keep_finished = 0;
}

static void go_background()
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

static void print_help(const char *cmd)
{
    printf("usage: %s [action] [-n] [-f] [cmd...]\n", cmd);
    printf("Actions:\n");
    printf("  -K       kill the task spooler server\n");
    printf("  -C       clear the list of finished jobs\n");
    printf("  -l       show the job list (default action)\n");
    printf("  -t [id]  tail -f the output of the job. Last if not specified.\n");
    printf("  -c [id]  cat the output of the job. Last if not specified.\n");
    printf("  -p [id]  show the pid of the job. Last if not specified.\n");
    printf("  -o [id]  show the output file. Of last job run, if not specified.\n");
    printf("  -r [id]  remove a job. The last added, if not specified.\n");
    printf("  -w [id]  wait for a job. The last added, if not specified.\n");
    printf("  -u [id]  put that job first. The last added, if not specified.\n");
    printf("  -h       show this help\n");
    printf("  -V       show the program version\n");
    printf("Options adding jobs:\n");
    printf("  -n       don't store the output of the command.\n");
    printf("  -f       don't fork into background.\n");
}

static void print_version()
{
    puts(version);
}

static void set_my_env()
{
    static char tmp[] = "POSIXLY_CORRECT=YES";
    putenv(tmp);
}

int main(int argc, char **argv)
{
    int errorlevel = 0;
    int jobid;

    set_my_env();
    /* This is needed in a gnu system, so getopt works well */
    default_command_line();
    parse_opts(argc, argv);

    if (command_line.need_server)
        ensure_server_up();

    switch(command_line.request)
    {
    case c_SHOW_VERSION:
        print_version(argv[0]);
        break;
    case c_SHOW_HELP:
        print_help(argv[0]);
        break;
    case c_QUEUE:
        assert(new_command != 0);
        assert(command_line.need_server);
        c_new_job(new_command);
        jobid = c_wait_newjob_ok();
        if (command_line.store_output)
            printf("%i\n", jobid);
        if (command_line.should_go_background)
        {
            go_background();
            c_wait_server_commands(new_command);
        } else
        {
            errorlevel = c_wait_server_commands(new_command);
        }

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
    case c_CAT:
        assert(command_line.need_server);
        c_cat();
        /* This will not return! */
        break;
    case c_SHOW_OUTPUT_FILE:
        assert(command_line.need_server);
        c_show_output_file();
        break;
    case c_SHOW_PID:
        assert(command_line.need_server);
        c_show_pid();
        break;
    case c_REMOVEJOB:
        assert(command_line.need_server);
        c_remove_job();
        break;
    case c_WAITJOB:
        assert(command_line.need_server);
        errorlevel = c_wait_job();
        break;
    case c_URGENT:
        assert(command_line.need_server);
        c_move_urgent();
        break;
    }

    if (command_line.need_server)
    {
        close(server_socket);
    }

    return errorlevel;
}

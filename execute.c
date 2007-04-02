/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "msg.h"
#include "main.h"

static void program_signal();

/* Returns errorlevel */
static int run_parent(int fd_read_filename, int pid)
{
    int status;
    int errorlevel;
    char *ofname = 0;
    int namesize;
    int res;

    /* Read the filename */
    /* This is linked with the write() in this same file, in run_child() */
    if (command_line.store_output) {
        res = read(fd_read_filename, &namesize, sizeof(namesize));
        if (res == -1)
        {
            perror("read filename");
            exit(-1);
        }
        assert(res == sizeof(namesize));
        ofname = (char *) malloc(namesize);
        res = read(fd_read_filename, ofname, namesize);
        assert(res == namesize);
    }
    close(fd_read_filename);

    c_send_runjob_ok(ofname, pid);

    wait(&status);

    if (WIFEXITED(status))
    {
        /* We force the proper cast */
        signed char tmp;
        tmp = WEXITSTATUS(status);
        errorlevel = tmp;
    } else
    {
        free(ofname);
        return -1;
    }

    if (command_line.send_output_by_mail)
    {
        char *command;
        command = build_command_string();
        send_mail(command_line.jobid, errorlevel, ofname, command);
        free(command);
    }

    free(ofname);

    return errorlevel;
}

void create_closed_read_on(int dest)
{
    int p[2];
    /* Closing input */
    pipe(p);
    close(p[1]); /* closing the write handle */
    dup2(p[0], dest); /* the pipe reading goes to stdin */
}

/* This will close fd_out and fd_in in the parent */
static void run_gzip(int fd_out, int fd_in)
{
    int pid;
    pid = fork();

    switch(pid)
    {
        case 0: /* child */
            dup2(fd_in,0); /* stdout */
            dup2(fd_out,1); /* stdout */
            close(fd_in);
            close(fd_out);
            /* Without stderr */
            close(2);
            execlp("gzip", "gzip", NULL);
            exit(-1);
            /* Won't return */
       case -1:
            exit(-1); /* Fork error */
       default:
            close(fd_in);
            close(fd_out);
    }
}

static void run_child(int fd_send_filename)
{
    char outfname[] = "/tmp/ts-out.XXXXXX";
    int namesize;
    int outfd;

    if (command_line.store_output)
    {
        int res;

        if (command_line.gzip)
        {
            int p[2];
            /* We assume that all handles are closed*/
            pipe(p);

            /* Program stdout and stderr */
            /* which go to pipe write handle */
            dup2(p[1], 1);
            dup2(p[1], 2);
            close(p[1]);

            /* gzip output goes to the filename */
            /* This will be the handle other than 0,1,2 */
            outfd = mkstemp(outfname); /* stdout */

            /* run gzip.
             * This wants p[0] in 0, so gzip will read
             * from it */
            run_gzip(outfd, p[0]);
        }
        else
        {
            /* Prepare the filename */
            outfd = mkstemp(outfname); /* stdout */
            dup2(outfd, 1); /* stdout */
            dup2(outfd, 2); /* stderr */
            close(outfd);
        }

        /* Send the filename */
        namesize = sizeof(outfname);
        res = write(fd_send_filename, (char *)&namesize, sizeof(namesize));
        write(fd_send_filename, outfname, sizeof(outfname));
    }
    close(fd_send_filename);

    /* Closing input */
    if (command_line.should_go_background)
        create_closed_read_on(0);

    execvp(command_line.command.array[0], command_line.command.array);
}

int run_job()
{
    int pid;
    int errorlevel;
    int p[2];


    /* For the parent */
    /*program_signal(); Still not needed*/

    /* Prepare the output filename sending */
    pipe(p);

    pid = fork();

    switch(pid)
    {
        case 0:
            close(server_socket);
            close(p[0]);
            run_child(p[1]);
            /* Not reachable, if the 'exec' of the command
             * works. Thus, command exists, etc. */
            exit(-1);
            break;
        case -1:
            perror("Error in fork");
            exit(-1);
            ;
        default:
            close(p[1]);
            errorlevel = run_parent(p[0], pid);
            break;
    }

    return errorlevel;
}

static void sigchld_handler(int val)
{
}

static void program_signal()
{
  struct sigaction act;

  act.sa_handler = sigchld_handler;
  /* Reset the mask */
  memset(&act.sa_mask,0,sizeof(act.sa_mask));
  act.sa_flags = SA_NOCLDSTOP;
  act.sa_restorer = NULL;

  sigaction(SIGCHLD, &act, NULL);
}

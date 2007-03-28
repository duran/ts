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
    free(ofname);

    wait(&status);

    if (WIFEXITED(status))
    {
        /* We force the proper cast */
        signed char tmp;
        tmp = WEXITSTATUS(status);
        errorlevel = tmp;
    } else
        return -1;

    return errorlevel;
}

static void run_gzip(int fd_in, int fd_out)
{
    int pid;
    pid = fork();

    switch(pid)
    {
        case 0: /* child */
            close(0); /* stdin */
            dup(fd_in);
            close(fd_in);
            close(1); /* stdout */
            dup(fd_out);
            close(fd_out);
            close(2); /* we definitely close stderr */
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

static void run_child(const char *command, int fd_send_filename)
{
    int p[2];
    char outfname[] = "/tmp/ts-out.XXXXXX";
    int namesize;
    int outfd;

    if (command_line.store_output)
    {
        int res;

        if (command_line.gzip)
        {
            int p[2];
            pipe(p);

            /* Program stdout ... */
            close(1);
            close(2); /* and stderr */
            /* ... goes to pipe write handle */
            dup(p[1]);
            dup(p[1]);
            close(p[1]);

            /* gzip output goes to the filename */
            outfd = mkstemp(outfname); /* stdout */

            /* run gzip.
             * This will take care of closing the handles */
            run_gzip(p[0], outfd);
        }
        else
        {
            close(1); /* stdout */
            close(2); /* stderr */
            /* Prepare the filename */
            outfd = mkstemp(outfname); /* stdout */
            dup(outfd); /* stderr */
        }

        /* Send the filename */
        namesize = sizeof(outfname);
        res = write(fd_send_filename, (char *)&namesize, sizeof(namesize));
        write(fd_send_filename, outfname, sizeof(outfname));
    }
    close(fd_send_filename);

    /* Closing input */
    pipe(p);
    close(p[1]); /* closing the write handle */
    close(0);
    dup(p[0]); /* the pipe reading goes to stdin */

    execlp("bash", "bash", "-c", command, NULL);
}

int run_job(const char *command)
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
            run_child(command, p[1]);
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

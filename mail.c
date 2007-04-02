#include <signal.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "main.h"

/* Returns the write pipe */
static int run_sendmail(const char *dest)
{
    int pid;
    int p[2];

    pipe(p);

    pid = fork();

    switch(pid)
    {
        case 0: /* Child */
            close(0);
            close(1);
            close(2);
            dup2(p[0], 0);
            execl("/usr/sbin/sendmail", "sendmail", "-oi", dest, NULL);
            exit(-1);
        case -1:
            perror("fork sendmail");
            exit(-1);
        default: /* Parent */
            ;
    }
    return p[1];
}

static void write_header(int fd, const char *dest, const char * command,
        int jobid, int errorlevel)
{
    FILE *f;
    int size;

    f = fdopen(fd, "a");
    assert(f != NULL);

    fprintf(f, "From: Task Spooler <taskspooler>\n", dest);
    fprintf(f, "To: %s\n", dest);
    fprintf(f, "Subject: the task %i finished with error %i. \n", jobid,
            errorlevel);
    fprintf(f, "\nCommand: %s\n", command);
    fprintf(f, "Output:\n");

    fflush(f);
}

static void copy_output(int write_fd, const char *ofname)
{
    int file_fd;
    char buffer[1000];
    int read_bytes;
    int res;

    file_fd = open(ofname, O_RDONLY);
    assert(file_fd != -1);

    do {
        read_bytes = read(file_fd, buffer, 1000);
        if (read_bytes > 0)
        {
            res = write(write_fd, buffer, read_bytes);
            assert(res != -1);
        }
    } while (read_bytes > 0);
    assert(read_bytes != -1);
}

void send_mail(int jobid, int errorlevel, const char *ofname,
    const char *command)
{
    char to[101];
    char *user;
    char *env_to;
    int write_fd;

    env_to = getenv("TS_MAILTO");

    if (env_to == NULL || strlen(env_to) > 100)
    {
        user = getenv("USER");
        if (user == NULL)
            user = "nobody";

        strcpy(to, user);
        /*strcat(to, "@localhost");*/
    } else
        strcpy(to, env_to);

    write_fd = run_sendmail(to);
    write_header(write_fd, to, command, jobid, errorlevel);
    copy_output(write_fd, ofname);
    close(write_fd);
}

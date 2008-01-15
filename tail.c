#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/time.h> /* Dep de main.h */

#include "main.h"

enum { BSIZE=1024 };

static int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

static void tail_error(const char *str)
{
    fprintf(stderr, "%s", str);
    fprintf(stderr, ". errno: %i (%s)\n",
                    errno, strerror(errno));
    exit(-1);
}

static void seek_at_last_lines(int fd, int lines)
{
    char buf[BSIZE];
    int lines_found = 0;
    int last_lseek = BSIZE;
    int last_read;
    int move_offset;
    int i;

    do
    {
        int next_read;
        next_read = min(last_lseek, BSIZE);
        last_lseek = lseek(fd, -BSIZE, SEEK_END);
        last_read = read(fd, buf, next_read);
        if (last_read == -1)
        {
            if (errno == EINTR)
                continue;
            tail_error("Error reading");
        }


        for(i = last_read-1; i >= 0; --i)
        {
            if (buf[i] == '\n')
            {
                ++lines_found;
                if (lines_found > lines)
                    break;
            }
        }
    } while(lines_found < lines);

    /* Calculate the position */
    if (lines_found > lines)
        i += 1;

    move_offset += i - last_read;
    lseek(fd, move_offset, SEEK_CUR);
}

void tail_file(const char *fname)
{
    int fd;
    int res;

    fd = open(fname, O_RDONLY);

    assert(fd != -1);

    seek_at_last_lines(fd, 10);

    do
    {
        char buf[BSIZE];
        int i;

        res = read(fd, buf, BSIZE);
        if (res == -1)
        {
            if (errno == EINTR)
            {
                res = 1; /* Hack for the while condition */
                continue;
            }
            tail_error("Error reading");
        }

        for(i=0; i < res; ++i)
        {
            putchar(buf[i]);
        }
    } while(res > 0);

    close(fd);
}

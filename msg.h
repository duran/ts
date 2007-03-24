extern int server_socket;

enum
{
    CMD_LEN=500,
    LINE_LEN=500
};

enum msg_types
{
    KILL,
    NEWJOB,
    NEWJOB_OK,
    RUNJOB,
    ENDJOB,
    LIST,
    LIST_LINE,
};

struct msg
{
    enum msg_types type;

    union
    {
        struct {
            int command_size;
        } newjob;
        int jobid;
        int errorlevel;
        char line[LINE_LEN];
    } u;
};


enum Jobstate
{
    QUEUED,
    RUNNING,
    FINISHED,
};

void send_bytes(const int fd, const char *data, const int bytes);
int recv_bytes(const int fd, char *data, const int bytes);
void send_msg(const int fd, const struct msg *m);
int recv_msg(const int fd, struct msg *m);

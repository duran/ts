extern int server_socket;

enum
{
    CMD_LEN=500,
    LINE_LEN=500
};

enum msg_types
{
    KILL_SERVER,
    NEWJOB,
    NEWJOB_OK,
    RUNJOB,
    RUNJOB_OK,
    ENDJOB,
    LIST,
    LIST_LINE,
    CLEAR_FINISHED,
    ASK_OUTPUT,
    ANSWER_OUTPUT
};

struct msg
{
    enum msg_types type;

    union
    {
        struct {
            int command_size;
            int store_output;
        } newjob;
        struct {
            int ofilename_size;
            int store_output;
            int pid;
        } output;
        int jobid;
        int errorlevel;
        int line_size;
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

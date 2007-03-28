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
    ANSWER_OUTPUT,
    REMOVEJOB,
    REMOVEJOB_OK,
    WAITJOB,
    WAITJOB_OK,
    URGENT,
    URGENT_OK,
    GET_STATE,
    ANSWER_STATE
};

enum Jobstate
{
    QUEUED,
    RUNNING,
    FINISHED
};

struct msg
{
    enum msg_types type;

    union
    {
        struct {
            int command_size;
            int store_output;
            int should_keep_finished;
        } newjob;
        struct {
            int ofilename_size;
            int store_output;
            int pid;
        } output;
        int jobid;
        int errorlevel;
        int line_size;
        enum Jobstate state;
    } u;
};

/* msg.c */
void send_bytes(const int fd, const char *data, const int bytes);
int recv_bytes(const int fd, char *data, const int bytes);
void send_msg(const int fd, const struct msg *m);
int recv_msg(const int fd, struct msg *m);

/* jobs.c */
const char * jstate2string(enum Jobstate s);

/* msgdump.c */
void msgdump(const struct msg *m);

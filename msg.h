extern int server_socket;

enum
{
    CMD_LEN=500
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
        char command[CMD_LEN];
        int jobid;
        int errorlevel;
        char command[LINE_LEN];
    } u;
};


enum Jobstate
{
    QUEUED,
    RUNNING,
    FINISHED,
};




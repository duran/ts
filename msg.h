extern int server_socket;

enum msg_types
{
    KILL
};

struct msg
{
    enum msg_types type;

    union
    {
        int data1;
        int data2;
    } u;
};


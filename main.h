enum Request
{
    c_QUEUE,
    c_TAIL,
    c_KILL_SERVER,
    c_LIST,
    c_CLEAR_FINISHED,
    c_SHOW_HELP,
    c_SHOW_VERSION,
    c_CAT,
    c_SHOW_OUTPUT_FILE,
    c_SHOW_PID,
    c_REMOVEJOB,
    c_WAITJOB,
    c_URGENT,
    c_GET_STATE,
    c_SWAP_JOBS
};

struct Command_line {
    enum Request request;
    int need_server;
    int store_output;
    int should_go_background;
    int should_keep_finished;
    int send_output_by_mail;
    int gzip;
    int jobid; /* When queuing a job, main.c will fill it automatically from
                  the server answer to NEWJOB */
    int jobid2;
    struct {
        char **array;
        int num;
    } command;
};

extern struct Command_line command_line;
extern int server_socket;

struct msg;

/* client.c */
void c_new_job();
void c_list_jobs();
void c_shutdown_server();
void c_wait_server_lines();
void c_clear_finished();
int c_wait_server_commands();
void c_send_runjob_ok(const char *ofname, int pid);
void c_tail();
void c_cat();
void c_show_output_file();
void c_remove_job();
void c_show_pid();
int c_wait_job();
void c_move_urgent();
int c_wait_newjob_ok();
void c_get_state();
void c_swap_jobs();
char *build_command_string();

/* jobs.c */
void s_list(int s);
int s_newjob(int s, struct msg *m);
void s_removejob(int jobid);
void job_finished(int errorlevel);
int next_run_job();
void s_mark_job_running();
void s_clear_finished();
void s_process_runjob_ok(int jobid, char *oname, int pid);
void s_send_output(int socket, int jobid);
void s_remove_job(int s, int jobid);
void s_remove_notification(int s);
void check_notify_list(int jobid);
void s_wait_job(int s, int jobid);
void s_move_urgent(int s, int jobid);
void s_send_state(int s, int jobid);
void s_swap_jobs(int s, int jobid1, int jobid2);

/* server.c */
void server_main(int notify_fd, char *_path);

/* server_start.c */
int try_connect(int s);
void wait_server_up();
int ensure_server_up();
void notify_parent(int fd);

/* execute.c */
int run_job();

/* client_run.c */
void c_run_tail(const char *filename);
void c_run_cat(const char *filename);

/* mail.c */
void send_mail(int jobid, int errorlevel, const char *ofname,
    const char *command);
void hook_on_finish(int jobid, int errorlevel, const char *ofname,
    const char *command);

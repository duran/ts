

struct msg;

/* client.c */
void c_new_job(const char *command, int store_output);
void c_list_jobs();
int c_shutdown_server();
void c_wait_server_lines();
int c_clear_finished();
void c_wait_server_commands(const char *my_command, int store_output);
void c_send_runjob_ok(int store_output, const char *ofname);

/* jobs.c */
void s_list(int s);
int s_newjob(int s, struct msg *m);
void s_removejob(int jobid);
void job_finished(int errorlevel);
int next_run_job();
void s_mark_job_running();
void s_clear_finished();
void s_process_runjob_ok(int jobid, char *oname);

/* msgdump.c */
void msgdump(const struct msg *m);

/* server.c */
void server_main();

/* server_start.c */
int try_connect(int s);
void wait_server_up();
int ensure_server_up();
void notify_parent(int fd);

/* execute.c */
int run_job(const char *command, int store_output);



struct msg;

/* client.c */
void c_new_job(const char *command);
void c_wait_server_commands();
void c_list_jobs(const char *command);
int c_shutdown_server();
void c_wait_server_lines();

/* jobs.c */
void s_list(int s);
int s_newjob(struct msg *m);
void s_removejob(int jobid);

/* msgdump.c */
void msgdump(const struct msg *m);

/* server.c */
void server_main();

/* server_start.c */
int try_connect(int s);
void wait_server_up();
void fork_server();
int ensure_server_up();

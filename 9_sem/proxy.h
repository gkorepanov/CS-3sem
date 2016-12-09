size_t get_max_fd_num();
size_t get_pipe_size();
size_t get_buf_size(size_t NODE);
void init(size_t NODE);
void free_structs();
void do_child(size_t MYNODE);
void do_write(size_t NODE, fd_set* readfds, fd_set* writefds);
void do_read(size_t NODE, fd_set* readfds, fd_set* writefds);
void transfer();

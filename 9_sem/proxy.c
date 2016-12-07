#include <math.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "tools/alerts.h"
#include <signal.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include "tools/tools.h"
#include <sys/select.h>
#include <fcntl.h>

// [n] is [number of chilldren] 
// [par] is [parent]
// [ch] is [child]

//==================================================================
//                         PROJECT CONSTANTS
//==================================================================

typedef int fd_t;
typedef char BYTE;
typedef char bool;

// some random values
#define MAX_CHILDREN 1000
#define MAX_BUFFER_SIZE 1e6
#define PIPE_SIZE 65536
#define BUF_CONST 4
#define BASIS 3 

// so as not to remember which end of pipe is read/write
#define READ 0
#define WRITE 1


// connection struct
typedef struct conn_t {
    pid_t child_pid;
    fd_t par[2];
    fd_t ch[2];
    size_t buf_size;
    size_t pipe_size;
    BYTE* buf;
    ssize_t data;
    bool active;
    bool closed;
    BYTE* data_ptr;
    BYTE* space_ptr;
} conn_t;


void init_conn(conn_t* conn, size_t number, size_t n) {
    fd_t tempfd1[2], tempfd2[2];
    ERRTEST(pipe(tempfd1))
    ERRTEST(pipe(tempfd2))

    conn->par[READ] = tempfd1[READ];
    conn->par[WRITE] = tempfd2[WRITE];


    conn->ch[READ] = tempfd2[READ];
    conn->ch[WRITE] = tempfd1[WRITE];


    // buf_size = C 3^(n_i)
    conn->buf_size = BUF_CONST * pow(BASIS, n-number);
    if (conn->buf_size > MAX_BUFFER_SIZE) 
        conn->buf_size = MAX_BUFFER_SIZE;

    /* conn->pipe_size = MIN(fcntl(conn->par[READ], F_GETPIPE_SZ),
                          fcntl(conn->par[WRITE], F_GETPIPE_SZ)); */

    conn->pipe_size = PIPE_SIZE;

    conn->buf = calloc(conn->buf_size, sizeof(BYTE));
    if (!conn->buf) 
        ERR("calloc() failed")

    conn->data_ptr = conn->buf;
    conn->space_ptr = conn->buf;

    conn->data = 0;
    conn->active = 0;
    conn->closed = 0;
}

void init_conns(conn_t* conns, size_t n, fd_t fd) {
    int i;
    
    // last connection is not associated with child, but with SRDOUT instead
    for (i = 0; i <= n; i++) {
        init_conn(&conns[i], i, n);
    }

    close(conns[0].ch[READ]);
    conns[0].ch[READ] = fd;
    conns[0].active = 1;

    close(conns[n].ch[WRITE]);
    close(conns[n].ch[READ]);
    close(conns[n].par[READ]);
    close(conns[n].par[WRITE]);

    conns[n].par[WRITE] = STDOUT_FILENO;
}

void destruct_conns(conn_t* conns, size_t n) {
    int i;
    for(i = 0; i < n; i++) {
        free(conns[i].buf);
    }
    free(conns);
}

void do_child(conn_t* conns, size_t number, size_t n) {
    int i;
    // closing connections from other children
    for(i = 0; i < n; i++) {
        if (i != number) {
            close(conns[i].par[READ]);
            close(conns[i].par[WRITE]);
            close(conns[i].ch[READ]);
            close(conns[i].ch[WRITE]);
        }
    }

    // closing unised parent's ends
    close(conns[number].par[READ]);
    close(conns[number].par[WRITE]);

    conn_t* conn = &conns[number];


    // reading data in blocking mode
    int bytes_read, bytes_send;

    errno = 0;
    while((bytes_read = 
            read(conn->ch[READ], conn->buf, conn->buf_size))) {

        if (errno) 
            ERR("Reading error in child, child number %zu", number)

        bytes_send = 0;

        // writing everything till no data is left
        while(bytes_send < bytes_read) {
            bytes_send += 
                write(conn->ch[WRITE], conn->buf + bytes_send, bytes_read - bytes_send);

            if (errno)
                ERR("Writing error in child, child number %zu", number)
        }
    }

    close(conn->ch[READ]);
    close(conn->ch[WRITE]);

    PRINT("closed connection, par read fd: %d", conn->par[READ])

    // free memory 
    destruct_conns(conns, n);   
    exit(EXIT_SUCCESS);
}

// called when accepting buffer is empty and then recalled till it is full
void do_read(conn_t* conns, size_t number, fd_set* def_readfds, fd_set* def_writefds) {
    conn_t* conn = &conns[number];
    conn_t* dest = &conns[number+1];
    if (!dest->active)
        dest->active = 1;

    int bytes_read;

    // trying to read whole buffer
    ERRTEST(bytes_read = read(conn->par[READ], dest->space_ptr, dest->buf_size - dest->data))

    PRINT("Read from child number %zu [%d bytes]", number, bytes_read)
#ifdef DEBUG
    printf("[");
    fflush(stdout);
    write(STDOUT_FILENO, dest->space_ptr, bytes_read);
    printf("]\n");
    fflush(stdout);
#endif

    if (!bytes_read) { // connection closed
        conn->active = 0;
        conn->closed = 1;
        // close fds, blocking reading and writing
        close(conn->par[READ]);
        FD_CLR(conn->par[READ], def_readfds);
        FD_CLR(conn->par[WRITE], def_writefds);

        PRINT("Closing connection with child %zu", number)
        
        FD_SET(dest->par[WRITE], def_writefds);
    }
    else { // some data read
        dest->data += bytes_read;
        dest->space_ptr += bytes_read;

        PRINT("dest buf size: %zu", dest->buf_size);
        PRINT("dest->data size: %zu", dest->data);
        PRINT("par read fd: %d", conn->par[READ]);

        if (dest->data == dest->buf_size) { // buffer is full
            PRINT("dest bufffer is full, sending request to write")
            dest->space_ptr = dest->buf;
            FD_CLR(conn->par[READ], def_readfds); // blocking read till empty again
            FD_SET(dest->par[WRITE], def_writefds); // start writing
        }
        
    }

    PRINT("fd_set for par[read]: %d", FD_ISSET(conn->par[READ], def_readfds))
    PRINT("child pid for corresponfing fd: %d", conn->child_pid);
}

// called when source buffer is full
void do_write(conn_t* conns, size_t number, fd_set* def_readfds, fd_set* def_writefds, int n) {
    conn_t* conn = &conns[number];
    conn_t* source = &conns[number-1];

    int bytes_write;

    errno = 0;
    // writing less then PIPE_SIZE bytes to be sure that pipe is big enough
    bytes_write = write(conn->par[WRITE], conn->data_ptr, MIN(conn->pipe_size, conn->data));
    

    PRINT("Write to child number %zu [%d bytes]", number, bytes_write)

    // if pipe is full, continue waiting for it to be available

    if (bytes_write == -1) {
        if (errno == EAGAIN)
            return;
        ERR("do_write() failed")
    }


    conn->data -= bytes_write;
    conn->data_ptr += bytes_write;

    // everything has been written
    if (!conn->data) {
        conn->data_ptr = conn->buf;

        if (source->active)
            FD_SET(source->par[READ], def_readfds); // now may fill the buffer
        
        if ((number != n) && (conns[number+1].active == 0))
            FD_SET(conn->par[READ], def_readfds); // now may read and involve next child into the process

        FD_CLR(conn->par[WRITE], def_writefds); // block writing till buffer is full

        if (source->active == 0) { // make child get eof to die
            PRINT("closing write fd to child %zu", number)
            close(conn->par[WRITE]);
        }
    }
    // else will try to write next time the rest of data
}


void transfer(conn_t* conns, size_t n) {
    fd_set def_readfds, def_writefds;
    fd_set readfds, writefds;
    FD_ZERO(&def_readfds);
    FD_ZERO(&def_writefds);

    int i, maxfd = 0;
    for (i = 0; i <= n; i++) {
        maxfd = MAX(maxfd, conns[i].par[READ]);
        maxfd = MAX(maxfd, conns[i].par[WRITE]);
    }

    FD_SET(conns[0].par[READ], &def_readfds);

    while(1) {
        readfds = def_readfds;
        writefds = def_writefds;

        select(maxfd + 1, &readfds, &writefds, 0, 0);


        for (i = 0; i <= n; i++) {
            if (FD_ISSET(conns[i].par[READ], &readfds))
                do_read(conns, i, &def_readfds, &def_writefds);

            if (FD_ISSET(conns[i].par[WRITE], &writefds))
                do_write(conns, i, &def_readfds, &def_writefds, n);
        }

        // check whether transfer is still going on
        if ((conns[n-1].closed) && !(conns[n].data))
            break;
        
    }
}



void give_birth(conn_t* conns, size_t n) {
    int i, fork_ret;

    for (i = 0; i < n; i++) {
        ERRTEST(fork_ret = fork())

        if (fork_ret == 0)  // child
            do_child(conns, i, n);
        else
            conns[i].child_pid = fork_ret;
    }
}


void remove_unused_fds(conn_t* conns, size_t n) {
    int i;
    for (i = 0; i < n; i++) {
        close(conns[i].ch[READ]);
        close(conns[i].ch[WRITE]);
    }
}

void switch_mode(conn_t* conns, size_t n) {
    int i;
    for (i = 0; i < n; i++) {
        fcntl(conns[i].par[WRITE], F_SETFL, O_NONBLOCK);
        fcntl(conns[i].par[READ], F_SETFL, O_NONBLOCK);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) 
       ERR("Usage: %s [n] [filename]\n", argv[0])
    
    fd_t fd;
    ERRTEST(fd = open(argv[2], O_RDONLY))

    long n = getnumber(argv[1]);
    
    conn_t* conns = calloc(n+1, sizeof(conn_t));

    if (!conns) 
        ERR("calloc() failed")

    init_conns(conns, n, fd);
    give_birth(conns, n);
    remove_unused_fds(conns, n);
    switch_mode(conns, n);

    transfer(conns, n);

    int i;
    for (i = 0; i < n; i++) 
        ERRTEST(waitpid(conns[i].child_pid, 0, 0))

    destruct_conns(conns, n);
}    

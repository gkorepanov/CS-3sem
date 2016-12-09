// local
#include "tools/alerts.h"
#include "tools/tools.h"
#include "tools/proxy_alerts.h"
#include "proxy.h"

// getrlimit()
#include <sys/time.h>
#include <sys/resource.h>

// open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


//==================================================================//
//                           ABBREVIATIONS                          //
//==================================================================//

// [n]      [number of children] 
// [par]    [parent]
// [ch]     [child]
// [nod]    [connectivity node]
// [NODE]   [current chnod/parnod number]

#define $PAR parnods[NODE-1]  // previous node
#define PAR  parnods[NODE]    // current node
#define PAR$ parnods[NODE+1]  // next node

#define $CH  chnods[NODE-1]
#define CH   chnods[NODE]
#define CH$  chnods[NODE+1]

#define FIRST 0
#define LAST  n-1

#define FOR_EACH_NODE for(size_t NODE = FIRST; NODE <= LAST; NODE++)

typedef int  fd_t;
typedef char byte_t;
typedef char bool;


//==================================================================//
//                              STRUCTS                             //
//==================================================================//

typedef struct parnod_t {
    fd_t in, out;       // fds referring to two consequent children:
                        // [i] and [i+1] respectively
    
    byte_t *buf, *end;   // buf_start, buf_end
    byte_t *head, *tail; // data boundaries
    
    bool on; // activity flag, switchs to 0 when data transfered
} parnod_t;

typedef struct chnod_t {
    fd_t in, out; 
} chnod_t;


//==================================================================//
//                              GLOBALS                             //
//==================================================================//

long n;
chnod_t* chnods;
parnod_t* parnods;


//==================================================================//
//                       TABLE OF CONVENTIONS                       //
//==================================================================//

/* Take STDIN, STDOUT, STDERR into account [-3]
   Take in account that initially 4 fd_t for each child are created,
   to be inherited [-2]
   Dividing by 2 as creating each node requires a pair of fd_t */
#define MAX_CHILDREN (get_max_fd_num() - 3 - 2)/2

// Size of par buffer[n] = MIN(BUF_CONST * BASIS**n, MAX_BUF_SIZE)
#define BUF_CONST           4
#define BASIS               3
#define MAX_BUF_SIZE        1e6 // just because
#define CHILD_BUF_SIZE      1e3 // just because

//  Aliases for fd_t[2] of pipes
#define READ    0
#define WRITE   1


//==================================================================//
//               COMMON STRUCTURE AND INITIALIZATIONS               //
//==================================================================//

int main(int argc, char* argv[]) {
    if (argc != (1 + 2)) 
       ERR("Usage: %s [n] [filename]", argv[0]);
    
    n = _strtolong(argv[1]);

    if (n > MAX_CHILDREN)
        ERR("Maximum number of children is %zu", MAX_CHILDREN);
    
    
    chnods  = calloc(n, sizeof(chnod_t));
    parnods = calloc(n, sizeof(parnod_t));

    if (!chnods || !parnods)
        ERR("calloc()");

    // acquiring fd for file and conveying it to the first child
    ERRTEST(chnods[FIRST].in = open(argv[2], O_RDONLY));

    // init parnods, produce children, remove redundant fds
    FOR_EACH_NODE
        init(NODE);
    transfer();

    free_structs();

    return 0;
}

void init(size_t NODE) {
    // init parnod
    size_t size = get_buf_size(NODE);
    PAR.buf =
        PAR.head =
            PAR.tail = (byte_t*) calloc(size, sizeof(byte_t));

    PAR.end = PAR.buf + size;
    PAR.on = 1;


    // init pipes
    fd_t pipe1[2], pipe2[2]; // temporary store fds
    ERRTEST(pipe(pipe1));
    ERRTEST(pipe(pipe2));

    CH.out = pipe1[WRITE];
    PAR.in = pipe1[READ];

    // already set input fd for the first child
    if (NODE == LAST) 
        // last parnod writes to stdout
        parnods[LAST].out = STDOUT_FILENO;  
    else {
        PAR.out = pipe2[WRITE];
        CH$.in  = pipe2[READ];
    }

    // give birth
    switch (fork()) {
        case -1:
            ERR("fork()");
        case 0: 
            do_child(NODE);
        default:
            // switch parent's fds to NONBLOCKing mode
            ERRTEST(fcntl(PAR.in, F_SETFL, O_NONBLOCK));
            ERRTEST(fcntl(PAR.out, F_SETFL, O_NONBLOCK));

            // close redundant children's fds
            ERRTEST(close(CH.in));
            ERRTEST(close(CH.out));
    }
}


//==================================================================//
//                            PARENT CODE                           //
//==================================================================//

void transfer() {
    // init fd_set
    fd_set readfds, writefds;
    fd_set mod_readfds, mod_writefds; // modified by select()
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    // determine maxfd
    size_t maxfd = 3; // MAX(STDERR, STDIN, STDOUT)
    FOR_EACH_NODE {
        maxfd = MAX(maxfd, PAR.in);
        maxfd = MAX(maxfd, PAR.out);
    }
    
    // initially all parnods are waiting for children to convey data
    FOR_EACH_NODE
        FD_SET(PAR.in, &readfds);


    // transferring info till all data is sent
    while(parnods[LAST].on) {
        mod_readfds = readfds;
        mod_writefds = writefds;

        // polling on pending fds
        select(maxfd + 1, &mod_readfds, &mod_writefds, 0, 0);

        FOR_EACH_NODE {
            if (FD_ISSET(PAR.in, &mod_readfds))
                do_read(NODE, &readfds, &writefds);
            if (FD_ISSET(PAR.out, &mod_writefds))
                do_write(NODE, &readfds, &writefds);
        }
    }
}

void do_read(size_t NODE, fd_set* readfds, fd_set* writefds) {
    ssize_t bytes_read;
    ERRTEST(bytes_read = read(PAR.in, PAR.tail, PAR.end - PAR.tail));
    PAR.tail += bytes_read;


#ifdef DEBUG
        PRINT("Read data: ");
        fflush(stdout);
        write(STDOUT_FILENO, PAR.head, PAR.tail - PAR.head);
        fflush(stdout);
#endif

    // end of data
    if (!bytes_read) {
        PRINT("CLOSING PAR.in number %zu", NODE);
        PAR.on = 0;
        close(PAR.in);
    }

    // buffer is full
    if ((PAR.tail == PAR.end) || !bytes_read) { 
        FD_CLR(PAR.in, readfds);
        FD_SET(PAR.out, writefds);
    }
}

void do_write(size_t NODE, fd_set* readfds, fd_set* writefds) {
    ERRTEST(errno ? -1 : 0); // just to make sure evrth was ok
    // (errno is checked beneath)

    PAR.head += write(PAR.out, PAR.head, PAR.tail - PAR.head);
    if (errno) ERR("write()");

    if (PAR.head == PAR.tail) { // everything written
        FD_CLR(PAR.out, writefds);

        // closing connection if there will be no more data
        if (!PAR.on) 
            close(PAR.out);
        else
            FD_SET(PAR.in, readfds);

    }
}


//==================================================================//
//                             CHILD CODE                           //
//==================================================================//

void do_child(size_t MYNODE) {
    // closing redundant parent's fds
    for (size_t NODE = FIRST; NODE <= MYNODE; NODE++) {
        ERRTEST(close(PAR.in));
        ERRTEST(close(PAR.out));
    }

    size_t NODE = MYNODE;

    // init local buffer and its params
    byte_t *buf, *head, *tail;

    buf =
        head =
            tail = (byte_t*) calloc(CHILD_BUF_SIZE, sizeof(byte_t));

    ERRTEST(errno ? -1 : 0); // just to make sure evrth was ok
    // (errno is checked beneath)


    size_t bytes_read;
    while ((bytes_read = read(CH.in, buf, CHILD_BUF_SIZE))) {
        if (errno) ERR("read()");
        tail = buf + bytes_read;

#ifdef DEBUG
        PRINTCH("Read data: ")
        fflush(stdout);
        write(STDOUT_FILENO, buf, tail - head);
        fflush(stdout);
#endif

        // write entire data (the plainest algorithm)
        while ((head += write(CH.out, head, tail - head)) != tail)
            if (errno) ERR("write()");

        PRINTCH("Written all data.");

        head = tail = buf;
    }
     
    if (errno)
        ERR("read()");

    ERRTEST(close(CH.in));
    ERRTEST(close(CH.out));
    free_structs(); 
    free(buf);

    exit(EXIT_SUCCESS); // it's very important to terminate here
}


//==================================================================//
//                           AUXILIARY CODE                         //
//==================================================================//

void free_structs() {
    FOR_EACH_NODE
        free(PAR.buf);

    free(parnods);
    free(chnods);
}


size_t get_max_fd_num() {
    struct rlimit rlim;
    ERRTEST(getrlimit(RLIMIT_NOFILE, &rlim))

    /* FROM MANPAGE:
        RLIMIT_NOFILE
        Specifies a value one greater than the maximum file
        descriptor number that can be opened by this process. */
    return (rlim.rlim_cur - 1);
}

size_t get_pipe_size() {
    /* here some code must have been written, however still no
       portable and reliable ways of determining pipe size found */
    return 65536;
}

size_t get_buf_size(size_t NODE) {
    size_t res = BUF_CONST;
    NODE = LAST - NODE; // buf's sizes grow in reverse order

    while(NODE-- && (res < MAX_BUF_SIZE)) 
        res *= BASIS;
        
    return res;
}

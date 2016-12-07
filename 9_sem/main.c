#include "tools/alerts.h"
#include "proxy.h"

#include <sys/time.h>
#include <sys/resource.h>

//==================================================================//
//                           ABBREVIATIONS                          //
//==================================================================//

// [n]      [number of children] 
// [par]    [parent]
// [ch]     [child]


#define $PAR parnods[i-1]
#define PAR  parnods[i]
#define PAR$ parnods[i+1]

#define $CH  chnods[i-1]
#define CH   chnods[i]
#define CH$  chnods[i+1]


//==================================================================//
//                              GLOBALS                             //
//==================================================================//

long n;
chnod_t* chnods;
parnods_t* parnods;


//==================================================================//
//                       TABLE OF CONVENTIONS                       //
//==================================================================//

typedef int  fd_t;
typedef char byte_t;
typedef char bool;


// Take STDIN, STDOUT, STDERR into account [-3]
// and dividing by 4 as creating each node requires a pair of fd_t[2]
#define MAX_CHILDREN (get_max_fd_num() - 3)/4

// Size of buffer[n] = min { BUF_CONST * BASIS**n; MAX_BUF_SIZE }
#define BUF_CONST           4
#define BASIS               3
#define MAX_BUF_SIZE        1e6

//  Aliases for int[2] of pipes
#define READ    0
#define WRITE   1


size_t get_max_fd_num() {
    struct rlimit rlim;
    ERRTEST(getrlimit(RLIMIT_NOFILE, &rlim))

    /* FROM MANPAGE:
        RLIMIT_NOFILE
        Specifies a value one greater than the maximum file descriptor
        number that can be opened by this process. */
    return (rlim.rlim_cur - 1);
}

size_t get_pipe_size() {
    // here some code must have been written, however still no portable
    // and reliable ways 
    return 65536;
}

size_t get_buf_size(size_t seq) {
    size_t res = BUF_CONST;
    seq = n - seq; // buf's sizes grow in reverse order

    while(seq-- && (res < MAX_BUF_SIZE)) 
        res *= BASIS;
        
    return res;
}


//==================================================================//
//                              STRUCTS                             //
//==================================================================//

typedef struct parnod_t {
    fd_t in, out;       // in and out fds refer to two consequent children:
                        // [i] and [i+1] respectively
    
    byte_t* buf, end;   // buf_start, buf_end
    byte_t* head, tail; // data boundaries
    
    bool active;
} parnod_t;

typedef struct chnod_t {/

    fd_t in, out; 
    
    byte_t* buf;
    size_t size; // buf_size
} chnod_t;


int main(int argc, char* argv[]) {
    if (argc != (1 + 2)) 
       ERR("Usage: %s [n] [filename]", argv[0]);
    
    n = _strtolong(argv[1]);

    if (n > MAX_CHILDREN)
        ERR("Maximum number of children is %d", MAX_CHILDREN);
    
    
    chnods  = calloc(n, sizeof(chnod_t));
    parnods = calloc(n, sizeof(parnod_t));

    if (!chnods || !parnods)
        ERR("calloc()");

    // acquiring fd for input file and conveying it to the first child
    ERRTEST(chnods[0].in = open(argv[2]));
    for (size_t i = 0; i < n; i++) {
        init(i); // init parnods, produce children, remove redundant fds
    }

    transfer();

    free_structs();

    return 0;
}


void init(size_t i) {
    // init parnod
    size_t size = get_buf_size(i);
    PAR.buf =
        PAR.head =
            PAR.tail = calloc(size, sizeof(byte_t));

    PAR.end = PAR.buf + size;
    PAR.active = 1;



    // init pipes
    fd_t pipe1[2], pipe2[2]; // temporary store fds
    ERRTEST(pipe(pipe1));
    ERRTEST(pipe(pipe2));

    CH.out = pipe1[WRITE];
    PAR.in = pipe1[READ];

    // first child will open file by himself
    if (i == (n-1)) 
        PAR.out = STDOUT_FILENO; // last parnod writes to stdout
    else {
        PAR.out = pipe2[WRITE];
        CH$.in  = pipe2[READ];
    }

    // give birth
    switch(fork()) {
        case -1:
            ERR("fork()");
        case 0: 
            do_child(i);
        default:
            // switch parent's fds to NONBLOCKing mode
            ERRTEST(fcntl(parnods[i].in, F_SETFL, O_NONBLOCK));
            ERRTEST(fcntl(parnods[i].out, F_SETFL, O_NONBLOCK));

            // close redundant children's fds
            ERRTEST(close(chnods[i].in));
            ERRTEST(close(chnods[i].out));
    }
}


void free_structs() {
    for (size_t i = 0; i < 0; i++) {
        free(PAR.buf);
        free(CH.buf); // free(NULL) invokes no actions
    }

    free(parnods);
    free(chnods);
}



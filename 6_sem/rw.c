#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "tools/alerts.h"
#include <signal.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <sys/sem.h>




//============================================================================
//                                 PROBLEMS
//============================================================================

// #1: Communication process. SEMAPHORES: (DATA, SPACE, MUTEX).

// #2: Servers mutual exclusion and checking operability.
// 1) if one server runs, other must terminate.
// 2) in case server dies, client must terminate as well.
// SEMAPHORES: (SERVER_EXIST)


// #3: Same as #2 for CLIENT. SEMAPHORES: (CLIENT_EXIST)

// #4: SEMAPHORES: (SERVER_EXISTED, CLIENT_EXISTED)
// The need for those semaphores is as follows.
// As soon as (CLIENT_EXIST) semaphore becomes ZERO after CLIENT death (#5), 
// new clients will not be able to know there were clients already (till 
// the server dies as well, when it would be just a new session).
// The way to handle that is to set (CLIENT_EXISTED) semaphore by SERVER after
// it has found the first client.


// #5: All crucial sem values (CLIENT EXIST, SERVER EXIST, DATA) must be
// increased with SEM_UNDO (otherwise in case of simultaneous death of server
// and client it would be impossible to 
// start new pair, as sem array won't be deleted and might contain wrong 
// values (non-zero ones)).


//============================================================================
//                             PROJECT CONSTANTS
//============================================================================

#define PROJ_ID 2874
#define SHM_SIZE 4096
#define DATA_SIZE SHM_SIZE - sizeof(int)
#define NAMESIZE 1000
#define SEMNUM 6
#define MAXOPSNUM 6

//============================================================================
//                       SEMAPHORES NAMES DEFINITIONS
//============================================================================

#define SERVER_EXIST 0 // 2 when server is started, 0 initially or in case
// it died, and 1 after client has connected
#define CLIENT_EXIST 1 // 2 when client is started, 0 initially or in case
// it died, and 1 after server has connected

#define SERVER_EXISTED 2 // 0 initially, 1 when there was a server (and
// probably died), set by client
#define CLIENT_EXISTED 3 // 0 initially, 1 when there was a client (and
// probably died), set by server

#define DATA 4 // data amount available to read in buffer
#define MUTEX 5 // mutual exclusion


//============================================================================
//                         FAST SEM OPERATION SET
//============================================================================

#define OP_SET(num, op, flags)\
    {\
        sops[opnum].sem_num = num;\
        sops[opnum].sem_op = op;\
        sops[opnum].sem_flg = flags;\
        opnum++;\
    }

#define SEM_DO\
    (\
        temp = opnum,\
        opnum=0,\
        semop(semid, sops, temp)\
    )

//============================================================================
//                             PROJECT CONSTANTS
//============================================================================

// TODO: check that struct is packed properly
struct data_to_send {
    int size;
    char data[DATA_SIZE];
};


//============================================================================
//                               SERVER CODE
//============================================================================

void do_server(char* file_name, int semid, void* memptr) {
    int fd, opnum = 0, temp; // for OP_SET, SEM_DO

    if ((fd = open(file_name, 0)) == -1) 
        ERR("ERROR: cannot open file %s.", file_name)
    PRINT("File opened.")

    struct sembuf sops[MAXOPSNUM];

    // Only one server might be running at the moment

    OP_SET(SERVER_EXIST, 0, IPC_NOWAIT) // check no servers are in presence
    OP_SET(SERVER_EXISTED, 0, IPC_NOWAIT) // check no servers already died
    OP_SET(SERVER_EXIST, 2, SEM_UNDO) // set 2, 1 is to show existence,
    // 1 is for client to take
    
    if (SEM_DO == -1) {
        if (errno == EAGAIN) 
            ERR("There had been already a server, terminating...")
        else
            ERR("Error running semop #1(): %s", strerror(errno))
    }
    else
        PRINT("Waiting for client...")

     
    // If here, we are the first and the only server
    //------------------------------------------------------------------------

    OP_SET(CLIENT_EXIST, -1, 0) // make sure client runs
    OP_SET(CLIENT_EXISTED, 1, SEM_UNDO) // set the sem indicating client existence

    if (SEM_DO == -1) 
        ERR("Error running semop #2(): %s", strerror(errno))
    else
        PRINT("Client connected.")


    // If here, connection established and all the sems are set properly
    //------------------------------------------------------------------------


    // Data transfer:
    struct data_to_send* data = (struct data_to_send*) memptr;

    int bytes_read;

    // Make sure in case server dies, DATA is 0 even in case sem array is not
    // deleted
    OP_SET(DATA, +1, SEM_UNDO)
    OP_SET(DATA, -1, 0)
    if (SEM_DO == -1) 
        ERR("Error running semop #2.1(): %s", strerror(errno))
        


    do {
        OP_SET(CLIENT_EXIST, -1, IPC_NOWAIT)
        OP_SET(CLIENT_EXIST, 1, 0)
        OP_SET(DATA, 0, 0)
        OP_SET(MUTEX, 0, 0) // Inversed MUTEX
        OP_SET(MUTEX, 1, SEM_UNDO) // UNDO makes sure it is 0 in case of death

        if (SEM_DO == -1) {
            if (errno == EAGAIN)
                ERR("Client has died, terminating...")
            else
                ERR("Error running semop #3(): %s", strerror(errno))
        }
        
        // Critical section in data transfer:

        if ((bytes_read = read(fd, data->data, DATA_SIZE)) == -1)
            ERR("read() error: %s", strerror(errno))

        data->size = bytes_read;
        

        OP_SET(MUTEX, -1, SEM_UNDO) // balancing UNDO

        // it's important here to use SEM_UNDO
        // to make sure DATA is 0 even in case sem arrsy is not deleted
        OP_SET(DATA, +1, 0)

        
        if (SEM_DO == -1) 
            ERR("Error running semop #4(): %s", strerror(errno))
    } while (bytes_read);


    close(fd);
};





//============================================================================
//                               CLIENT CODE
//============================================================================

void do_client(int semid, void* memptr) {
    int opnum = 0, temp; // for OP_SET, SEM_DO

    struct sembuf sops[MAXOPSNUM];

    // Only one client might be running at the moment

    OP_SET(CLIENT_EXIST, 0, IPC_NOWAIT) // check no clients are in presence
    OP_SET(CLIENT_EXISTED, 0, IPC_NOWAIT) // check no clients already died
    OP_SET(CLIENT_EXIST, 2, SEM_UNDO) // set 2, 1 is to show existence,
    // 1 is for server to take

    
    if (SEM_DO == -1) {
        if (errno == EAGAIN) 
            ERR("There had been already a client, terminating...")
        else
            ERR("Error running semop #1(): %s", strerror(errno))
    }
    else
        PRINT("Waiting for server...")

     
    // If here, we are the first and the only client
    //------------------------------------------------------------------------


    OP_SET(SERVER_EXIST, -1, 0) // make sure server runs
    OP_SET(SERVER_EXISTED, 1, SEM_UNDO) // set the sem indicating server existence

    if (SEM_DO == -1) 
        ERR("Error running semop #2(): %s", strerror(errno))
    else
        PRINT("Server connected.")


    // If here, connection established and all the sems are set properly
    //------------------------------------------------------------------------


    // Data transfer:
    struct data_to_send* data = (struct data_to_send*) memptr;

    int bytes_write;

    do {
        OP_SET(SERVER_EXIST, -1, IPC_NOWAIT)
        OP_SET(SERVER_EXIST, 1, 0)
        OP_SET(DATA, -1, 0)
        OP_SET(MUTEX, 0, 0)
        OP_SET(MUTEX, 1, SEM_UNDO)

        if (SEM_DO == -1) {
            if (errno == EAGAIN)
                ERR("Server has died, terminating...")
            else
                ERR("Error running semop #3(): %s", strerror(errno))
        }
        
        // Critical section in data transfer:

        bytes_write = data->size;
        if (write(STDOUT_FILENO, data->data, bytes_write) == -1)
            ERR("read() error: %s", strerror(errno))

        OP_SET(MUTEX, -1, SEM_UNDO) 
        
        if (SEM_DO == -1) 
            ERR("Error running semop #4(): %s", strerror(errno))
    } while (bytes_write);

};








//============================================================================
//                                  MAIN CODE
//============================================================================

int main(int argc, char *argv[]) {
    int is_server;

    if (argc == 1) 
        is_server = 0;

    else if (argc == 2) {
        is_server = 1;
    }

    else 
        ERR("Usage: %s [file (if server)]", argv[0])
    
   
    int key = ftok("/tmp", PROJ_ID);
    int shmid, semid;
    if((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) == -1)
        ERR("shmget error: %s", strerror(errno))
    
    if((semid = semget(key, SEMNUM, IPC_CREAT | 0666)) == -1)
        ERR("semget error: %s", strerror(errno))
    
    PRINT("semid: %d; key: %d", semid, key)

    void* memptr = NULL;
    if ((memptr = shmat(shmid, NULL, 0)) == ((void *) -1))
        ERR("shmat error: %s", strerror(errno))


    if (is_server)
        do_server(argv[1], semid, memptr);
    else
        do_client(semid, memptr);

    PRINT("Done, terminating.")

    shmdt(memptr);

    // The client is the one who removes the IPC resources, as it
    // terminates after server
    if (!is_server) {
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
    }

    exit(EXIT_SUCCESS);
}

// --------------------------------------------------------------------------
//                                  CODE
//---------------------------------------------------------------------------

    mkfifo("fifo", 666);

    int fd = open("fifo", O_RDWR);
    write(fd, "AAA", 3);

    close(fd);

    char buf[3];
    int fd = open("fifo", O_RDONLY | O_NDELAY);

    write(1, buf, read(fd, buf, 3);


// --------------------------------------------------------------------------
//                                  ANSWER
//---------------------------------------------------------------------------

/*
    1) Simple user: 
                    both open() fail (Permissions)
                    both write & read() fail (decriptors are -1)

    2) SUDO'ers:
                    read() fails with 0 as another end is already closed
                    write() writes exactly 0 bytes to STDOUT

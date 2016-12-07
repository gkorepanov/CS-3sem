int is_same_file(int fd1, int fd2) {
    struct stat stat1, stat2;
    if(fstat(fd1, &stat1) < 0) 
        return -1;
    if(fstat(fd2, &stat2) < 0) 
        return -1;

    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

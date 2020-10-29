#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int parent_fd[2];
    int child_fd[2];
    int pid;
    pipe(parent_fd);
    pipe(child_fd);
    pid = fork();
    if (pid == 0) // Child process
    {
        char message[5];
        read(parent_fd[0], message, 5);
        printf("%d: received %s\n", getpid(), message);
        write(child_fd[1], "pong", 5);
    }
    else if (pid > 0)
    {
        char message[5];
        write(parent_fd[1], "ping", 5);
        wait();
        read(child_fd[0], message, 5);
        printf("%d: received %s\n", getpid(), message);
    }
    exit();
}

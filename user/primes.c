#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void pipeline(int pipe_out)
{
    uint8 number;
    uint8 output;
    int fd[2];
    if (read(pipe_out, &number, 1) <= 0)
    {
        close(pipe_out);
        return;
    }
    printf("prime %d\n", number);
    pipe(fd);
    if (fork() == 0)
    {
        close(fd[1]);
        int new_fd = dup(fd[0]);
        close(fd[0]);
        pipeline(new_fd);
    }
    else
    {
        close(fd[0]);
        output = number;
        while (read(pipe_out, &number, 1) > 0)
            if (number % output != 0)
                write(fd[1], &number, 1);
        close(fd[1]);
        wait();
    }
}

int main(int argc, char *argv[])
{
    int parent_fd[2];
    uint8 i; // Integer sent
    pipe(parent_fd);
    if (fork() == 0)
    {
        close(parent_fd[1]);
        int new_fd = dup(parent_fd[0]);
        close(parent_fd[0]);
        pipeline(new_fd);
    }
    else
    {
        close(parent_fd[0]);
        for (i = 2; i <= 35; ++i)
            write(parent_fd[1], &i, 1);
        close(parent_fd[1]);
        wait();
    }
    exit();
}

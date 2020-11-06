#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_SIZE 256
#define MAX_ARGS 32

int main(int argc, char* argv[])
{
    char ch;
    char buf[MAX_SIZE];
    char arg_buf[MAX_ARGS + 1][MAX_SIZE];
    char* args[MAX_ARGS + 1];
    char in_red_buf[MAX_SIZE]; // Input redirection
    int in_red_exists;
    int in_red_flag; // Makrs whether next argument belongs to input redirection
    char* p;
    char* ptr_in_red;
    char* temp;
    int i;
    int status;
    int fd;
    for (i = 0; i < MAX_ARGS + 1; ++i)
        args[i] = arg_buf[i];
    p = buf;
    ptr_in_red = in_red_buf;
    i = 0;
    in_red_exists = 0;
    in_red_flag = 0;
    printf("@ ");
    while (read(0, &ch, 1) > 0)
    {
        switch (ch)
        {
            case ' ':
                if (p == buf)
                    continue;
                if (in_red_flag && ptr_in_red == in_red_buf)
                    continue;
                if (in_red_flag)
                {
                    *ptr_in_red = '\0';
                    ptr_in_red = in_red_buf;
                    in_red_flag = 0;
                }
                else
                {
                    *p = '\0';
                    p = buf;
                    if (i < MAX_ARGS)
                    {
                        strcpy(args[i++], buf);
                    }
                }
                break;
            case '\n':
                if (i == 0 && p == buf)
                    continue;
                if (in_red_flag && ptr_in_red == in_red_buf)
                {
                    fprintf(2, "nsh: no file to redirect standard input to\n");
                    continue;
                }
                if (in_red_flag)
                {
                    *ptr_in_red = '\0';
                    ptr_in_red = in_red_buf;
                    in_red_flag = 0;
                }
                else
                {
                    *p = '\0';
                    p = buf;
                    if (i < MAX_ARGS)
                    {
                        strcpy(args[i++], buf);
                    }
                }
                temp = args[i];
                args[i] = 0;
                if (fork() == 0)
                {
                    if (in_red_exists)
                    {
                        close(0);
                        if ((fd = open(in_red_buf, O_RDONLY)) < 0)
                        {
                            fprintf(2, "nsh: open %s failed\n", in_red_buf);
                            exit(-1);
                        }
                    }
                    if (exec(args[0], args) < 0)
                        fprintf(2, "nsh: exec %s failed\n", args[0]);
                    exit(0);
                }
                else
                {
                    args[i] = temp;
                    i = 0;
                    in_red_exists = 0;
                    in_red_flag = 0;
                    wait(&status);
                    printf("@ ");
                }
                break;
            case '<':
                if (p != buf) // Handle previous argument
                {
                    *p = '\0';
                    p = buf;
                    if (i < MAX_ARGS)
                    {
                        strcpy(args[i++], buf);
                    }
                }
                in_red_exists = 1;
                in_red_flag = 1;
                break;
            default:
                if (in_red_flag)
                    *ptr_in_red++ = ch;
                else
                    *p++ = ch;
                break;
        }
    }
    exit(0);
}
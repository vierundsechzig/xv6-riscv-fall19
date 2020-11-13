#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_SIZE 256
#define MAX_ARGS 32

void execcmd(char* cmd)
{
    static char buf[MAX_SIZE];
    static char arg_buf[MAX_ARGS + 1][MAX_SIZE];
    static char* args[MAX_ARGS + 1];
    static char in_red_buf[MAX_SIZE]; // Input redirection
    int in_red_exists;
    int in_red_flag; // Makrs whether next argument belongs to input redirection
    int in_red_error;
    static char out_red_buf[MAX_SIZE]; // Output redirection
    int out_red_exists;
    int out_red_flag; // Makrs whether next argument belongs to output redirection
    int out_red_error;
    char* ptr_cmd;
    char* p;
    char* ptr_in_red;
    char* ptr_out_red;
    char* temp;
    int i;
    int status;
    int fd;
    for (i = 0; i < MAX_ARGS + 1; ++i)
        args[i] = arg_buf[i];
    p = buf;
    ptr_in_red = in_red_buf;
    ptr_out_red = out_red_buf;
    i = 0;
    in_red_exists = 0;
    in_red_flag = 0;
    in_red_error = 0;
    out_red_exists = 0;
    out_red_flag = 0;
    out_red_error = 0;
    for (ptr_cmd = cmd; *ptr_cmd != '\0'; ++ptr_cmd)
    {
        switch (*ptr_cmd)
        {
            case ' ':
                // Initial blanks are ignored
                if (!in_red_flag && !out_red_flag && p == buf)
                    continue;
                if (in_red_flag && ptr_in_red == in_red_buf)
                    continue;
                if (out_red_flag && ptr_out_red == out_red_buf)
                    continue;
                if (in_red_flag)
                {
                    *ptr_in_red = '\0';
                    ptr_in_red = in_red_buf;
                    in_red_flag = 0;
                }
                else if (out_red_flag)
                {
                    *ptr_out_red = '\0';
                    ptr_out_red = out_red_buf;
                    out_red_flag = 0;
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
                continue;
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
                else if (out_red_flag && ptr_out_red == out_red_buf) // No file to redirect input to
                {
                    out_red_error = 1; // Error in input redirection
                }
                else if (out_red_flag)
                {
                    *ptr_out_red = '\0';
                    ptr_out_red = out_red_buf;
                    out_red_flag = 0;
                }
                in_red_exists = 1;
                in_red_flag = 1;
                break;
            case '>':
                if (p != buf) // Handle previous argument
                {
                    *p = '\0';
                    p = buf;
                    if (i < MAX_ARGS)
                    {
                        strcpy(args[i++], buf);
                    }
                }
                else if (in_red_flag && ptr_in_red == in_red_buf) // No file to redirect input to
                {
                    in_red_error = 1; // Error in input redirection
                }
                else if (in_red_flag)
                {
                    *ptr_in_red = '\0';
                    ptr_in_red = in_red_buf;
                    in_red_flag = 0;
                }
                out_red_exists = 1;
                out_red_flag = 1;
                break;
            default:
                if (in_red_flag)
                    *ptr_in_red++ = *ptr_cmd;
                else if (out_red_flag)
                    *ptr_out_red++ = *ptr_cmd;
                else
                    *p++ = *ptr_cmd;
                break;
        }
    }
    if (i == 0 && p == buf)
        return;
    if ((in_red_flag && ptr_in_red == in_red_buf) || in_red_error)
    {
        fprintf(2, "nsh: no file to redirect standard input to\n");
        return;
    }
    if ((out_red_flag && ptr_out_red == out_red_buf) || out_red_error)
    {
        fprintf(2, "nsh: no file to redirect standard output to\n");
        return;
    }
    if (in_red_flag)
    {
        *ptr_in_red = '\0';
        ptr_in_red = in_red_buf;
        in_red_flag = 0;
    }
    else if (out_red_flag)
    {
        *ptr_out_red = '\0';
        ptr_out_red = out_red_buf;
        out_red_flag = 0;
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
        if (out_red_exists)
        {
            close(1);
            if ((fd = open(out_red_buf, O_WRONLY|O_CREATE)) < 0)
            {
                fprintf(2, "nsh: create %s failed\n", out_red_buf);
                exit(-1);
            }
        }
        if (exec(args[0], args) < 0)
            fprintf(2, "nsh: exec %s failed\n", args[0]);
        exit(0);
    }
    else
    {
        // Parent process: ready for next line
        args[i] = temp;
        i = 0;
        in_red_exists = 0;
        in_red_flag = 0;
        in_red_error = 0;
        out_red_exists = 0;
        out_red_flag = 0;
        out_red_error = 0;
        wait(&status);
    }
}

int main(int argc, char* argv[])
{
    char cmd[MAX_SIZE];
    do
    {
        printf("@ ");
        if (*gets(cmd, MAX_SIZE) != '\0')
            execcmd(cmd);
        else
            break;
    } while (1);
    exit(0);
}
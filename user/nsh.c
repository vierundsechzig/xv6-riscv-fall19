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
    int in_red_flag; // Marks whether next argument belongs to input redirection
    int in_red_error; // Set on syntax error
    static char out_red_buf[MAX_SIZE]; // Output redirection
    int out_red_exists;
    int out_red_flag; // Marks whether next argument belongs to output redirection
    int out_red_error; // Set on syntax error
    int pipe_flag;
    int out_red_before_pipe;
    int in_red_after_pipe;
    int pipe_error; // Set on trying to set a pipe of more than two elements
    int cmd_left_argc; // Argument count of the command before pipe
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
    pipe_error = 0;
    pipe_flag = 0;
    out_red_before_pipe = 0;
    in_red_after_pipe = 0;
    cmd_left_argc = 0;
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
                if (pipe_flag)
                    in_red_after_pipe = 1;
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
                if (!pipe_flag)
                    out_red_before_pipe = 1;
                break;
            case '|':
                if (pipe_flag) // Already exists a '|'
                    pipe_error = 1;
                else
                {
                    pipe_flag = 1;
                    cmd_left_argc = i;
                    if (!in_red_flag && !out_red_flag && p == buf)
                        continue;
                    if (in_red_flag && ptr_in_red == in_red_buf)
                        in_red_error = 1;
                    else if (out_red_flag && ptr_out_red == out_red_buf)
                        out_red_error = 1;
                    else if (in_red_flag)
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
                }
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
    if (pipe_error)
    {
        fprintf(2, "nsh: sorry, we do not support pipe of more than two elements\n");
        return;
    }
    if (pipe_flag)
    {
        if (out_red_before_pipe)
        {
            fprintf(2, "nsh: output redirection should not come before pipe\n");
            return;
        }
        if (in_red_after_pipe)
        {
            fprintf(2, "nsh: input redirection should not come after pipe\n");
            return;
        }
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
    if (pipe_flag)
    {
        int pipe_before, pipe_after;
        int transfer[2];
        if (pipe(transfer) < 0)
        {
            fprintf(2, "nsh: intializing pipe failed\n");
            exit(-1);
        }
        pipe_before = fork();
        if (pipe_before == 0)
        {
            temp = args[cmd_left_argc];
            args[cmd_left_argc] = 0;
            if (in_red_exists)
            {
                close(0);
                if ((fd = open(in_red_buf, O_RDONLY)) < 0)
                {
                    fprintf(2, "nsh: open %s failed\n", in_red_buf);
                    exit(-1);
                }
            }
            close(1);
            dup(transfer[1]);
            close(transfer[0]);
            close(transfer[1]);
            if (exec(args[0], args) < 0)
                fprintf(2, "nsh: exec %s failed\n", args[0]);
            exit(0);
        }
        pipe_after = fork();
        if (pipe_after == 0)
        {
            temp = args[i];
            args[i] = 0;
            if (out_red_exists)
            {
                close(1);
                if ((fd = open(out_red_buf, O_WRONLY|O_CREATE)) < 0)
                {
                    fprintf(2, "nsh: open %s failed\n", out_red_buf);
                exit(-1);
                }
            }
            close(0);
            dup(transfer[0]);
            close(transfer[0]);
            close(transfer[1]);
            if (exec(args[cmd_left_argc], args + cmd_left_argc) < 0)
                fprintf(2, "nsh: exec %s failed\n", args[cmd_left_argc]);
            exit(0);
        }
        close(transfer[0]);
        close(transfer[1]);
        wait(&status);
        wait(&status);
    }
    else
    {
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
                    fprintf(2, "nsh: open %s failed\n", out_red_buf);
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
            wait(&status);
        }
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

#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[])
{
    char* args[MAXARG];
    char buf[128];
    char* p;
    char* temp;
    char ch;
    int i;
    p = buf;
    for (i = 1; i < MAXARG - 1; ++i)
        args[i] = (char*) malloc(128);
    for (i = 1; i < argc; ++i)
        strcpy(args[i], argv[i]);
    while (read(0, &ch, 1) > 0)
    {
        if (ch == '\n')
        {
            if (p == buf)
                continue;
            *p = '\0';
            p = buf;
            if (i < MAXARG - 1)
            {
                strcpy(args[i++], buf);
            }
            temp = args[i];
            args[i] = 0;
            if (fork() == 0)
            {
                exec(args[1], args + 1);
                exit();
            }
            else
            {
                args[i] = temp;
                i = argc;
                wait();
            }
        }
        else if (ch == ' ')
        {
            if (p == buf)
                continue;
            *p = '\0';
            p = buf;
            if (i < MAXARG - 1)
            {
                strcpy(args[i++], buf);
            }
        }
        else
        {
            *p++ = ch;
        }
    }
    for (i = 1; i < MAXARG - 1; ++i)
        free(args[i]);
    exit();
}
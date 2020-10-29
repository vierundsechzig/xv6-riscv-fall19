#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int ticks;
    if (argc == 2)
    {
        ticks = atoi(argv[1]);
	    sleep(ticks);
    }
    else if (argc < 2)
    {
        printf("sleep: too few arguments");
    }
    else
    {
        printf("sleep: too many arguments");
    }
    exit();
}

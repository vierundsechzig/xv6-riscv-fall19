#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define MAX_LENGTH 512

char* filename(char* path)
{
    char* p;
    for (p = path + strlen(path); p >= path && *p != '/'; --p)
        ;
    ++p;
    return p;
}

int find(char* dir, const char* file)
{
    char buf[MAX_LENGTH];
    char* p;
    int fd;
    int result;
    struct dirent de;
    struct stat st;
    if ((fd = open(dir, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", dir);
        return 0;
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        return 0;
    }
    switch (st.type)
    {
        case T_FILE:
            if (strcmp(filename(dir), file) == 0)
            {
                printf("%s\n", dir);
                close(fd);
                return 1;
            }
            close(fd);
            return 0;
        case T_DIR:
            if (strlen(dir) + 1 + DIRSIZ + 1 > MAX_LENGTH)
            {
                printf("find: path too long\n");
                close(fd);
                return 0;
            }
            strcpy(buf, dir);
            p = buf + strlen(buf);
            *p++ = '/';
            result = 0;
            while (read(fd, &de, sizeof de) == sizeof de)
            {
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                result |= find(buf, file);
            }
            close(fd);
            return result;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc == 3 && !find(argv[1], argv[2]))
    {
        printf("find: file not found\n");
    }
    else if (argc != 3)
    {
        printf("find: too many or too few arguments\n");
    }
    exit();
}

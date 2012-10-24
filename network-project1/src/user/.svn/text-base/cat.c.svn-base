// A simple cat program for GeekOS

#include <conio.h>
#include <process.h>
#include <fileio.h>

int main(int argc, char **argv) {
    int ret;
    int read;
    int inFd;
    struct VFS_File_Stat stat;
    char buffer[1025];

    if (argc != 2) {
        Print("usage: cat <file>\n");
        Exit(-1);
    }

    inFd = Open(argv[1], O_READ);
    if (inFd < 0) {
        Print("unable to open %s\n", argv[1]);
        Exit(-1);
    }

    ret = FStat(inFd, &stat);
    if (ret != 0) {
        Print("error stating file\n");
    }
    if (stat.isDirectory) {
        Print("cp can not copy directories\n");
        Exit(-1);
    }

    for (read = 0; read < stat.size; read += ret) {
        ret = Read(inFd, buffer, sizeof(buffer) - 1);
        if (ret < 0) {
            Print("error reading file for copy\n");
            Exit(-1);
        }

        buffer[ret] = '\0';
        Print("%s", buffer);
    }

    Close(inFd);
    return 0;
}

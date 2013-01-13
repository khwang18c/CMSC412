// A test program for GeekOS user mode

#include <conio.h>
#include <process.h>
#include <fileio.h>
#include <geekos/projects.h>

int main(int argc, char **argv) {
  
#if PROJECT_GFS2 
    Print("Formatting...\n");
    int pid;
    pid = Spawn_With_Path("gfs2f.exe", "gfs2f.exe ide1 10", "/c:/a");
    if (Wait(pid) >= 0) {
        Print("Mounting...\n");
        if (Mount("ide1", "/d", "gosfs") >= 0) {
            Print("Writing...\n");
            int fd = Open("/d/testWrite", O_WRITE | O_CREATE);
            if (fd >= 0) {
                char buffer[100] =
                    "Hello.  If you see this your write works.\n";
                if (Write(fd, buffer, 100) == 100)
                    Print("Wrote file /d/testWrite\n");
            }
            Print("Sync...\n");
            Sync();
            Print("Done sync\n");
        }
    }
#endif

    return 0;
}

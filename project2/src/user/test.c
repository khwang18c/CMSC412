#include <syscall.h>
#include <process.h>
#include <geekos/signal.h>
#include <conio.h>

#define PATH "/c:/a"

void test1(void);
void test2(void);
void test3(void);

int main(int argc, char **argv) {
    if (argc == 1){
        Spawn_With_Path("test", "test 1", PATH, 0);
        Spawn_With_Path("test", "test 2", PATH, 0);
        while(1);
    } else if (argc == 2 && atoi(argv[1]) == 1){
        while(1);
    } else if (argc == 2 && atoi(argv[1]) == 2) {
        Kill(10, SIGKILL);
        Kill(11, SIGKILL);
    }
    return 0;
}

void sh1 (void) {
    Print("Test sh1!\n");
}

void sh2 (void) {
    Print("Test sh2!\n");
}

void child_handler(void) {
    int status;
    int zombiepid;
    while ((zombiepid = WaitNoPID(&status)) >= 0) {
        Print("Exit status of zombie: %d\n", status);
        Print("Zombiepid: %d\n", zombiepid);
    }
    return;
}

void test1(void) {
    int rc = 0;
    rc += Signal(&sh1, SIGUSR1);
    rc += Kill(10, SIGUSR1);
    
    rc += Signal(&sh2, SIGUSR2);
    rc += Kill(10, SIGUSR2);
    Print("test1 RC: %d\n", rc);
}

void test2(void) {
    int rc = 0;

    Signal(&sh1, SIGUSR1); 
    Spawn_With_Path("test", "test 1", PATH, 0);
}

void test3(void) {
     
    //Signal(&child_handler, SIGCHLD);
    Print("PID %d started\n", Spawn_With_Path("test", "test 2", PATH, 0));

    Print("PID %d started\n", Spawn_With_Path("test", "test 2", PATH, 0));
    
    Print("PID %d started\n", Spawn_With_Path("test", "test 2", PATH, 0));
} 
void test4(void) {
    
}    

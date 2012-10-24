#include <conio.h>
#include <syscall.h>
#include <geekos/kthread.h>

int main(int argc, char ** argv) {
    Print("Time start: %d\n", Get_Time_Of_Day());
    int policy = 1;
    Set_Scheduling_Policy(policy, 10);
    int i;
    for (i = 0; i < 5000000000; i++){
    }

    Print("Time end: %d\n", Get_Time_Of_Day());
    
    return 0;
}


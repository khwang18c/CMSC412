#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>

int main( int argc , char ** argv )
{

  int holdsched3_sem;

  holdsched3_sem = Open_Semaphore("holdsched3_sem",0);

  P(holdsched3_sem);
  Print("3");
  V(holdsched3_sem);

  Close_Semaphore(holdsched3_sem);
  return 0;
}

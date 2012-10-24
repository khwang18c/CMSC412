// A test program for semaphores

#include "libuser.h"
#include "conio.h"

int main( int argc, char ** argv)
{
  int semkey, result;

  Print("Open_Semaphore()...\n");
  semkey = Open_Semaphore("semtest", 3);
  Print("Open_Semaphore() returned %d\n", semkey);

  if (semkey < 0)
    return 0;

  Print("P()...\n");
  result = P(semkey);
  Print("P() returned %d\n", result);

  Print("P()...\n");
  result = P(semkey);
  Print("P() returned %d\n", result);

  Print("V()...\n");
  result = V(semkey);
  Print("V() returned %d\n", result);


  Print("Close_Semaphore()...\n");
  result = Close_Semaphore(semkey);
  Print("Close_Semaphore() returned %d\n", result);

  return 0;
}

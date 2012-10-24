#include <conio.h>
// #include <process.h>
extern int Spawn_Program(const char *program, const char *command, int background);
#include <sched.h>
#include <sema.h>
#include <string.h>

#if !defined (NULL)
#define NULL 0
#endif

int main( int argc , char ** argv )
{
  int scr_sem,holdp3_sem;	/* sid of screen semaphore */
  int id1, id2, id3;    	/* ID of child process */
  
  holdp3_sem = Open_Semaphore ("holdp3_sem", 0);
  scr_sem    = Open_Semaphore ( "screen" , 1 )  ;
  

  P ( scr_sem ) ;
  Print ("Semtest1 begins\n");
  V ( scr_sem ) ;


  id3 = Spawn_Program ( "/c/sem-p3.exe", "/c/sem-p3.exe", 0 ) ;
  P ( scr_sem ) ;
  Print ("p3 created\n");
  V ( scr_sem ) ;
  id1 = Spawn_Program ( "/c/sem-p1.exe", "/c/sem-p2.exe", 0 ) ;
  id2 = Spawn_Program ( "/c/sem-p2.exe", "/c/sem-p1.exe", 0 ) ;
  

  Wait(id1);
  Wait(id2);
  Wait(id3);

  Close_Semaphore(scr_sem);
  Close_Semaphore(holdp3_sem);
  return 0;
}


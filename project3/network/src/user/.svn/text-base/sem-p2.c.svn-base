#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>

int main( int argc , char ** argv )
{
  int i ;     	/* loop index */
  int scr_sem; 		/* id of screen semaphore */
  int prod_sem, cons_sem;
  int holdp3_sem;

  scr_sem = Open_Semaphore ( "screen" , 1 ) ;   /* register for screen use */
  prod_sem = Open_Semaphore ( "prod_sem" , 0 ) ;    
  cons_sem = Open_Semaphore ( "cons_sem" , 1 ) ;   
  holdp3_sem = Open_Semaphore ( "holdp3_sem", 0 ) ;

  for (i=0; i < 5; i++) {
    P(prod_sem);
    Print ("Consumed %d\n",i) ;
    V(cons_sem);
  }
  
  V(holdp3_sem);
  
  Close_Semaphore(scr_sem);
  Close_Semaphore(prod_sem);
  Close_Semaphore(cons_sem);
  Close_Semaphore(holdp3_sem);
  return 0;
}

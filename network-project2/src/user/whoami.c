// A user id test program for GeekOS user mode

#include <conio.h>
#include <fileio.h>
#include <process.h>

void main( int argc, char *argv[] )
{
    Print("my uid = %d\n", GetUid());
}

/*
 * Automated test program for Project 5
 * Copyright (c) 2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2004 Iulian Neamtiu <neamtiu@cs.umd.edu>
 * $Revision: 1.3 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>
#include <string.h>
#include <geekos/projects.h>

char bigbuffer[10000] = {};

int doTest( const char * testName, int (*testFunction)(), int points, int * score, int * totalTests, int * successfulTests)
{
  int ret;

  (*totalTests)++;

  Print("Testing: %s...", testName);

  ret = testFunction();

  if (ret < 0 )
    Print ("FAILED (%d)", ret);
  else
    {
      Print ("PASSED (%d)", ret);
      (*score) += points;
      (*successfulTests)++;
    }

  Print (" crt score: %d \n", (*score));

  return ret;

}

int ttestFormat()
{
  if(PROJECT_GOSFS) {
    return Format("ide1", "gosfs");
  } else {
    Print("skipped format test for gfs2 project");
    return 0;
  }
}

int ttestMount()
{
  if(PROJECT_GFS2) {
    return Mount( "ide1", "/d", "gfs2" );
  } else {
    return Mount( "ide1", "/d", "gosfs" ); 
  }
}

int tOpenInexistentFile()
{
  return (Open("/d/InexistentFile", O_READ) < 0) ? 1 : -1;
}

int tCreat()
{
  int retC;

  int fd = Open("/d/somefile", O_CREATE|O_READ);
  if (fd < 0)
    return -1;

  retC = Close(fd);

  return (retC >= 0) ? 1 : -1;
}

int tCreatLongFilename()
{
  int retC;

  int fd = Open("/d/somePrettyLongFileNameToBeCreated", O_CREATE|O_READ);
  if (fd < 0)
    return -1;

  retC = Close(fd);

  return (retC >= 0) ? 1 : -1;
}

int tCreatInexistentPath()
{
  int fd;

  fd = Open("/d/InexistentPath/file", O_CREATE|O_READ);

  return  (fd < 0) ? 1 : -1;
}

int tBasicReadWrite()
{
  int fd, retW, retR;
  char buffer[100];

  fd = Open("/d/basic", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  Close(fd);

  fd = Open("/d/basic", O_READ);
  if (fd < 0)
    return -1;

  retR = Read( fd, buffer, 10);
  if (retR < 0)
    return -1;

  Close(fd);

  return ( (retW == 10) || (retR == 10) ) ? 1 : -1;
}


int tReadFromWriteOnly()
{
  int fd, retR, retW;
  char buffer[100];

  fd = Open("/d/basic2f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  Close(fd);

  fd = Open("/d/basic2f", O_WRITE);
  if (fd < 0)
    return -1;

  retR = Read( fd, buffer, 10);

  Close(fd);

  return (retR < 0)  ? 1 : -1;
}

int tWriteToReadOnly()
{
  int fd, retW;
  char buffer[100];

  fd = Open("/d/basic3f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  Close(fd);

  fd = Open("/d/basic3f", O_READ);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);

  Close(fd);

  return (retW < 0)  ? 1 : -1;
}

int tCloseTwice()
{
  int fd, retC;

  fd = Open("/d/basic4f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  Close(fd);

  retC = Close(fd);

  return (retC < 0)  ? 1 : -1;
}


int tCloseAberrantFd()
{
  int retC;

  retC = Close(100000);

  return (retC < 0)  ? 1 : -1;
}

int tBasicDelete()
{
  int fd, retD;

  fd = Open("/d/basic5f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  Close(fd);

  retD = Delete("/d/basic5f");

  return (retD >= 0)  ? 1 : -1;
}

int tDeleteInexistentFile()
{
  int retD;

  retD = Delete("/d/InexistentFile2");

  return (retD < 0)  ? 1 : -1;
}

int tBasicCreateDirectory()
{
  int retC;

  retC = Create_Directory("/d/dir1d");

  return (retC >= 0)  ? 1 : -1;
}

int tRecursivelyCreateDirectory()
{
  int retC;

  retC = Create_Directory("/d/dir2d");

  if (retC < 0) {
    Print("failed to create directory /d/dir2d: %d\n", retC);
    return -1;
  }

  retC = Create_Directory("/d/dir2d/dir3d");
  if (retC < 0) {
    Print("failed to create directory /d/dir2d/dir3d: %d\n", retC);
    return -1;
  }

  return (retC >= 0)  ? 1 : -1;
}

int tFileInRecursivelyCreatedDirectory()
{
  int fd = Open("/d/dir2d/dir3d/file4f", O_CREATE|O_READ);
  Close(fd);

  return (fd >= 0) ? 1 : -1;
}


int tDeleteEmptyDirectory()
{
  int retC, retD;

  retC = Create_Directory("/d/dir3d");
  if (retC < 0)
    return -1;

  retD = Delete("/d/dir3d");

  return (retD >= 0)  ? 1 : -1;
}


int tDeleteNonEmptyDirectory()
{
  int retC, retD, fd;

  retC = Create_Directory("/d/dir4d");
  if (retC < 0)
    return -1;

  fd = Open("/d/dir4d/file", O_CREATE|O_READ);
  if (fd < 0)
    return -1;

  Close(fd);

  retD = Delete("/d/dir4d");

  return (retD >= 0) ? -1 : 1;
}

int tBasicSeek()
{
  int fd, retW, retS1, retS2;
  char buffer[11];

  fd = Open("/d/basic6f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  retS1 = Seek(fd, 0);
  retS2 = Seek(fd, 9);

  Close(fd);
  Delete("/d/basic6f");

  return ( (retS1 >= 0) && (retS2 >= 0) ) ? 1 : -1;
}

int tSeekReread()
{
  int fd, retW, retR, retS;
  char buffer[11]="0123456789\0", buffer2[2], buffer3[2];

  fd = Open("/d/basic7f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  Close(fd);

  fd = Open("/d/basic7f", O_READ);
  if (fd < 0)
    return -1;

  retS = Seek(fd, 0);
  if (retS < 0)
    return -1;

  retR = Read( fd, buffer2, 1);
  if (retR < 0)
    return -1;

  retS = Seek(fd, 9);
  if (retS < 0)
    return -1;

  retR = Read( fd, buffer3, 1);
  if (retR < 0)
    return -1;

  Close(fd);
  Delete("/d/basic7f");

  return ( (buffer2[0] == '0') && (buffer3[0] == '9') ) ? 1 : -1;
}

int tBasicStat()
{
  int fd, retS;
  struct VFS_File_Stat s;

  fd = Open("/d/basic8f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  Close(fd);

  fd = Open("/d/basic8f", O_READ);
  if (fd < 0)
    return -1;

  retS = FStat(fd, &s);

  Close(fd);
  Delete("/d/basic8f");

  return (retS >= 0) ? 1 : -1;
}

int tStatFile()
{
  int fd, retW, retS;
  char buffer[11];
  struct VFS_File_Stat s;

  fd = Open("/d/basic9f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  Close(fd);

  retS = Stat("/d/basic9f", &s);

  Delete("/d/basic9f");

  return ( (retS >= 0) && (s.size == 10) ) ? 1 : -1;
}

int tStatDirectory()
{
  int fd, retS, retC;
  struct VFS_File_Stat s;

  retC = Create_Directory("/d/basic10d");
  if (retC < 0)
    return -1;

  fd = Open_Directory("/d/basic10d");
  if (fd < 0)
    return -1;

  retS = FStat(fd, &s);

  Close(fd);
  Delete("/d/basic10d");

  return ( (retS >= 0) && (s.isDirectory)  ) ? 1 : -1;
}

int tReadEntry()
{
  int fd, retR, retC;
  struct VFS_Dir_Entry dirEntry;

  retC = Create_Directory("/d/basic11d");
  if (retC < 0) {
    Print("couldn't create basic11d: %d\n", retC);
    return -1;
  }

  retC = Create_Directory("/d/basic11d/d1");
  if (retC < 0) {
    Print("couldn't create basic11d/d1: %d\n", retC);
    return -1;
  }

  retC = Create_Directory("/d/basic11d/d2");
  if (retC < 0) {
    Print("couldn't create basic11d/d2: %d\n", retC);
    return -1;
  }

  fd = Open("/d/basic11d/f1", O_CREATE);
  if (fd < 0) {
    Print("couldn't create basic11d/f1: %d\n", fd);
    return -1;
  }

  Close(fd);

  fd = Open_Directory("/d/basic11d");
  if (fd < 0) {
    Print("couldn't opendir basic11d: %d\n", fd);
    return -1;
  }

  do {
    retR = Read_Entry(fd, &dirEntry);
  } while(retR == 0 && dirEntry.name[0] == '.');

  if ((retR < 0) ||
      (strncmp(dirEntry.name, "d1", 2) != 0) ||
       (! dirEntry.stats.isDirectory))
    return -1;

  retR = Read_Entry(fd, &dirEntry);

  if ((retR < 0) ||
      (strncmp(dirEntry.name, "d2", 2) != 0) ||
       (! dirEntry.stats.isDirectory))
    return -1;

  retR = Read_Entry(fd, &dirEntry);

  if ((retR < 0) ||
      (strncmp(dirEntry.name, "f1", 2) != 0) ||
       (dirEntry.stats.isDirectory))
    return -1;
  
  Close(fd);

  fd = Open_Directory("/d/basic11d");
  if (fd < 0)
    return -1;

  retR = Seek(fd, 2);
  if (retR < 0)
    return -1;

  retR = Read_Entry(fd, &dirEntry);

  if ((retR < 0) ||
      (strncmp(dirEntry.name, "f1", 2) != 0) ||
       (dirEntry.stats.isDirectory))
    return -1;
  
  Close(fd);
  Delete("/d/basic11d/d1");
  Delete("/d/basic11d/d2");
  Delete("/d/basic11d/f1");

  Delete("/d/basic11d");
  return 1;
}


int tFiveMegs()
{
  int fd, retW, retR, retS;
  int i,j;
  char buffer[1000], buffer2[1000];
  int ret = 1;

  for (i = 0; i < 1000; i++ )
    buffer[i] = buffer2[i] = i & 0xff;

  fd = Open("/d/bigfile", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  Print("Writing in first 5MB at random ... \n");

  for (i = 0; i < 50; i++ )
    {
      //retS = Seek(fd, 100000 * i);

      /* nspring didn't comment out the above line, but did comment out the stuff that tests 
         whether the return value was okay 
      if (retS < 0 )
	{
	  ret = -1;
	  break;
	}
      */

      retW = Write( fd, buffer, 1000);
      if (retW != 1000)
	{
	  ret = -1;
	  break;
	}
      Print(" %d", i);
    }

  Close(fd);

  if (ret < 0)
    {
      Delete("/d/bigfile");
      return -1;
    }

  Print("Reading back from first 5MB ... \n");

  fd = Open("/d/bigfile", O_READ);

  if (fd < 0)
    return -1;

  for (i = 0; i < 50; i++ )
    {
      //retS = Seek(fd, 100000 * i);
      retS = 0; /* no, I don't know what this is about, or why the call to seek has been commented, but 
                   it does seem like retS should be initialized. -ns */

      if (retS < 0 )
	{
	  ret = -1;
	  break;
	}

      retR = Read( fd, buffer, 1000);
      if (retR != 1000)
	{
	  ret = -1;
	  break;
	}

      for (j = 0; j < 1000; j++ )
	{
	  if (buffer[j] != buffer2[j])
	    {
	      ret = -1;
	      break;
	    }
	}

      Print(" %d", i);
    }

  Close(fd);

  Delete("/d/bigfile");

  return ret;
}

int tWriteReread(int howManyKBs, char const * fileName)
{
  int fd, retW, retR;
  int i, j;
  char buffer[100], buffer2[100];
  int ret = 1;

  //Print("====BUFFER====\n");
  for (j = 0; j < 100; j++ ) {
    buffer[j] = j;
//    Print("buffer[%d] = %d\n", j, buffer[j]);
  }
/*
  int fd1 = Open("/d/somefile", O_CREATE|O_READ);
  Print("fd1 = %d\n", fd1);
  Close(fd1);
  
  Create_Directory("/d/somedir");

  int fd2 = Open("/d/basic", O_CREATE|O_WRITE);
  Print("fd2 = %d\n", fd2);
  int ret1 = Write(fd2, bigbuffer, 10000);
  if (ret1 != 10000)
    Print("ret1 failed\n");
  Close(fd2);

  char buf2[10] = {};
  int fd3 = Open("/d/basic7f", O_CREATE|O_WRITE);
  Print("fd3 = %d\n", fd3);
  int ret2 = Write(fd3, buf2, 10);
  if (ret2 != 10)
    Print("ret2 failed\n");
  Close(fd3);

  Create_Directory("/d/basic11d");
  Create_Directory("/d/basic11d/d1");
  Create_Directory("/d/basic11d/d2");
  int fd4 = Open("/d/basic11d/f1", O_CREATE|O_WRITE);
  Print("fd4 = %d\n", fd4);
  int ret3 = Write(fd4, bigbuffer, 10000);
  if (ret3 != 10000)
    Print("ret3 failed\n");
  Close(fd4);

  Create_Directory("/d/recursive_stat1");
  Create_Directory("/d/recursive_stat1/recursive_stat2");
  Create_Directory("/d/recursive_stat1/recursive_stat2/recursive_stat3");
  int fd5 = Open("/d/recursive_stat1/recursive_stat2/recursive_stat3/recursive_stat4", O_CREATE|O_WRITE);
  int ret4 = Write(fd5, bigbuffer, 10000);
  Close(fd5);*/
 
  fd = Open(fileName, O_WRITE|O_CREATE);
  if (fd < 0)
    return -1;

  for (i = 0; i < (howManyKBs * 10); i++ )
    {
      retW = Write( fd, buffer, 100);
      if (retW != 100)
	{
	  ret = -1;
	  break;
	}
      if (i%50 == 0)
	Print(" %d", i);
    }

  struct VFS_File_Stat *stat = Malloc(sizeof(struct VFS_File_Stat));
  FStat(fd, stat);
  Close(fd);

  if (ret != -1)
    {
      fd = Open(fileName, O_READ);
      if (fd < 0)
	return -1;

      for (i = 0; i < (howManyKBs * 10); i++ )
	{
	  retR = Read( fd, buffer2, 100);

	  if (retR != 100)
	    {
	      ret = -1;
	      break;
	    }

	  for(j = 0; j < 100; j++)
	    {
	      if (buffer2[j] != j)
		{
		  ret = -1;
		  break;
		}
	    }
	  
	  if (ret < 0 )
	    break;

	  if (i%50 == 0)
	    Print(" %d", i);
	}

      Close(fd);
      Delete(fileName);
    }

  return ret;
}

int t10KWriteReread()
{
  return tWriteReread(10, "/d/file_10k");
}

int t100KWriteReread()
{
  return tWriteReread(100, "/d/file_100k");
}


int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{


  int score = 0; int totalTests = 0; int successfulTests = 0;

  // 0
  if( PROJECT_GOSFS )
    doTest( "Format", ttestFormat, 3, &score, &totalTests, &successfulTests);

  // 1
  doTest( "Mount", ttestMount, 1,  &score, &totalTests, &successfulTests);
  // 2
  
  doTest( "Open-Inexistent File", tOpenInexistentFile, 1,  &score, &totalTests, &successfulTests);
  // 3
  doTest( "Creat", tCreat, 2,  &score, &totalTests, &successfulTests);
  // 4
  doTest( "Creat-Long Filename", tCreatLongFilename, 3,  &score, &totalTests, &successfulTests);
  // 5
  doTest( "Creat-Inexistent Path", tCreatInexistentPath, 3,  &score, &totalTests, &successfulTests);
  // 6
  doTest( "Basic Read/Write", tBasicReadWrite, 5,  &score, &totalTests, &successfulTests);

  // 7
  doTest( "Read from WriteOnly", tReadFromWriteOnly, 3,  &score, &totalTests, &successfulTests);
  // 8
  doTest( "Write to ReadOnly", tWriteToReadOnly, 3,  &score, &totalTests, &successfulTests);

  // 9
  doTest( "Close Twice", tCloseTwice, 3,  &score, &totalTests, &successfulTests);
  // 10
  doTest( "Close Illegal FD", tCloseAberrantFd, 1,  &score, &totalTests, &successfulTests);

  // 11
  doTest( "Basic Delete", tBasicDelete , 3,  &score, &totalTests, &successfulTests);
  // 12
  doTest( "Delete-Inexistent File", tDeleteInexistentFile, 1,  &score, &totalTests, &successfulTests);

  // 13
  doTest( "Basic Create Directory", tBasicCreateDirectory, 3,  &score, &totalTests, &successfulTests);
  // 14
  doTest( "Recursively Create Directory", tRecursivelyCreateDirectory, 5,  &score, &totalTests, &successfulTests);
  // 15
  doTest( "Create File in Recursively Created Directory", tFileInRecursivelyCreatedDirectory, 3,  &score, &totalTests, &successfulTests);

  // 16
  doTest( "Basic Seek", tBasicSeek, 2,  &score, &totalTests, &successfulTests);
  // 17
  doTest( "Seek w/ Reread", tSeekReread, 5,  &score, &totalTests, &successfulTests);

  // 18
  doTest( "Basic Stat", tBasicStat, 2,  &score, &totalTests, &successfulTests);
  // 19
  //doTest( "Stat-File", tStatFile, 2,  &score, &totalTests, &successfulTests);
  // 20
  doTest( "Stat-Directory", tStatDirectory, 2,  &score, &totalTests, &successfulTests);

  // 21
  doTest( "Delete Empty Directory", tDeleteEmptyDirectory, 3,  &score, &totalTests, &successfulTests);
  // 22
  doTest( "Delete Non-Empty Directory", tDeleteNonEmptyDirectory, 2,  &score, &totalTests, &successfulTests);

  // 23
  doTest( "10k Write/Reread", t10KWriteReread, 5,  &score, &totalTests, &successfulTests);

  // 24
  doTest( "100k Write/Reread", t100KWriteReread, 7,  &score, &totalTests, &successfulTests);
  // 25
  doTest( "Read Entry", tReadEntry, 4,  &score, &totalTests, &successfulTests);
  // 26

  doTest( "5 MB Write", tFiveMegs, 8,  &score, &totalTests, &successfulTests);

  Print ("********************************************\n");
  Print ("Tests attempted: %d. Passed: %d. Failed: %d\n", totalTests, successfulTests, (totalTests-successfulTests) );
  Print ("SCORE: %d\n", score);

  return 0;
}


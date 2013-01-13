/*
 * Automated test program for Project 5x
 * Copyright (c) 2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2004 Iulian Neamtiu <neamtiu@cs.umd.edu>
 * Copyright (c) 2008 Neil Spring <nspring@cs.umd.edu>
 * $Revision: 1.10 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>
#include <string.h>
#include <geekos/gfs2.h>
#include <geekos/vfs.h>
#include <geekos/projects.h>

#define doTestOrDie(t,f,p,s,tt,st) do { if(doTest(t,f,p,s,tt,st) < 0) { Exit(1); } } while(0)

unsigned int blocksize=4096;
unsigned int disksize_mb=2;
bool fail_immediately=false;
#define VFS_NO_MORE_DIR_ENTRIES 1

int doTest( const char * testName, 
            int (*testFunction)(), int points, int * score, int * totalTests, int * successfulTests)
{
  int ret;

  (*totalTests)++;

  Print("Testing: %s...", testName);

  ret = testFunction();

  if (ret < 0 ) {
    Print ("FAILED (%d)", ret);
    if(fail_immediately) {
      Exit(-1);
    }
  } else {
      Print ("PASSED (%d)", ret);
      (*score) += points;
      (*successfulTests)++;
    }

  Print (" crt score: %d \n", (*score));

  return ret;

}

int ttestFormat()
{
  
    int pid;
  char commandline[250];
  if(false) {
    // (void)snprintf(commandline, 249, "gfs2f.exe ide1 %u %u", disksize_mb, blocksize);
    // pid = Spawn_With_Path("gfs2f.exe", commandline, "/c:/a", 0);
    // return Wait(pid);
  } else {
    return 0;
  }

}

int ttestMount()
{
  return Mount( "ide1", "/d", "gfs2" );
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
  
  (void)Delete("/d/somefile");

  return (retC >= 0) ? 1 : -1;
}

int tCreatLongFilename()
{
  int retC;

  int fd = Open("/d/somePrettyLongFileNameToBeCreated", O_CREATE|O_READ);
  if (fd < 0)
    return -1;

  retC = Close(fd);
  
  (void)Delete("/d/somePrettyLongFileNameToBeCreated");

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

  memset(buffer, 'g', 100);

  fd = Open("/d/basic", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);
  if (retW < 0) {
	Print("write return %d < 10\n", retW);
    return -1;
  }

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open("/d/basic", O_READ);
  if (fd < 0) {
	Print("couldn't reopen for reading: %d %s\n", fd, Get_Error_String(fd));
    return -1;
  }

  retR = Read( fd, buffer, 10);
  if (retR < 0) {
	Print("read return %d < 10\n", retR);
    return -1;
  }

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  
  (void)Delete("/d/basic");

  return ( (retW == 10) || (retR == 10) ) ? 1 : -1;
}


int tReadFromWriteOnly()
{
  int fd, retR, retW;
  char buffer[100];

  fd = Open("/d/basic2f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  memset(buffer, 'n', sizeof(buffer)); 
  
  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open("/d/basic2f", O_WRITE);
  if (fd < 0)
    return -1;

  retR = Read( fd, buffer, 10);

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  
  (void)Delete("/d/basic2f");


  return (retR < 0)  ? 1 : -1;
}

int tWriteToReadOnly()
{
  int fd, retW;
  char buffer[100];

  fd = Open("/d/basic3f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  memset(buffer, 'n', sizeof(buffer));
  retW = Write( fd, buffer, 10);
  if (retW < 0)
    return -1;

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open("/d/basic3f", O_READ);
  if (fd < 0)
    return -1;

  retW = Write( fd, buffer, 10);

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  
  (void)Delete("/d/basic3f");


  return (retW < 0)  ? 1 : -1;
}

int tCloseTwice()
{
  int fd, retC;

  fd = Open("/d/basic4f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  retC = Close(fd);
  
  (void)Delete("/d/basic4f");

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
  int fd, retD, retS;
  struct VFS_File_Stat s;

  fd = Open("/d/basic5f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  retD = Delete("/d/basic5f");
  
  retS = Stat("/d/basic5f", &s);

  return (retD >= 0 && retS < 0)  ? 1 : -1;
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
  
  (void)Delete("/d/dir1d");

  return (retC >= 0)  ? 1 : -1;
}

int tRecursivelyCreateDirectory()
{
  int retC;

  retC = Create_Directory("/d/dir2d");

  if (retC < 0) 
    return -1;

  retC = Create_Directory("/d/dir2d/dir3d");

  return (retC >= 0)  ? 1 : -1;
}

int tFileInRecursivelyCreatedDirectory()
{
  int fd = Open("/d/dir2d/dir3d/file4f", O_CREATE|O_READ);
  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  
  (void)Delete("/d/dir2d/dir3d/file4f");
  (void)Delete("/d/dir2d/dir3d");
  (void)Delete("/d/dir2d");


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

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  retD = Delete("/d/dir4d");
  
  (void)Delete("/d/dir4d/file");
  (void)Delete("/d/dir4d");

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

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  (void)Delete("/d/basic6f");

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

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

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

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  (void)Delete("/d/basic7f");

  return ( (buffer2[0] == '0') && (buffer3[0] == '9') ) ? 1 : -1;
}

int tBasicStat()
{
  int fd, retS;
  struct VFS_File_Stat s;

  fd = Open("/d/basic8f", O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open("/d/basic8f", O_READ);
  if (fd < 0)
    return -1;

  retS = FStat(fd, &s);

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  (void)Delete("/d/basic8f");

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

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  retS = Stat("/d/basic9f", &s);

  (void)Delete("/d/basic9f");

  return ( (retS >= 0) && (s.size == 10) ) ? 1 : -1;
}

int tStatDirectory()
{
  int fd, retS, retC;
  struct VFS_File_Stat s;

  retC = Create_Directory("/d/basic10d");
  if (retC < 0) {
    Print("couldn't create basic10d: %d %s\n", retC, Get_Error_String(retC));
    return -1;
  }

  fd = Open_Directory("/d/basic10d");
  if (fd < 0) {
    Print("couldn't reopen basic10d: %d %s\n", fd, Get_Error_String(fd));
    return -1;
  }

  retS = FStat(fd, &s);

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  (void)Delete("/d/basic10d");

  if(retS < 0) {
    Print("couldn't fstat opened basic10d: %d %s\n", retS, Get_Error_String(retS));
  }
  if(!s.isDirectory) {
    Print("fstat didn't think basic10d was a directory\n");
  }


  return ( (retS >= 0) && (s.isDirectory)  ) ? 1 : -1;
}

int tRecursiveStat() {
	int retC, retS, fd;
	struct VFS_File_Stat s;
	
	retC = Create_Directory("/d/recursive_stat1");
	if (retC < 0) return -1;
	
	retC = Create_Directory("/d/recursive_stat1/recursive_stat2");
	if (retC < 0) return -1;

	retC = Create_Directory("/d/recursive_stat1/recursive_stat2/recursive_stat3");
	if (retC < 0) return -1;

	fd = Open("/d/recursive_stat1/recursive_stat2/recursive_stat3/recursive_stat4", O_CREATE|O_WRITE);
	if (fd < 0) return -1;
	
	retC = Close(fd);

	retS = Stat("/d/recursive_stat1", &s);
	if(retS < 0) return -1;

	retS = Stat("/d/recursive_stat1x", &s);
	if(retS == 0) return -1;

	retS = Stat("/d/recursive_stat1/recursive_stat2", &s);
	if(retS < 0) return -1;
	
	retS = Stat("/d/recursive_stat1x/recursive_stat2", &s);
	if(retS == 0) return -1;
	
	retS = Stat("/d/recursive_stat1/../recursive_stat1", &s);
	if(retS < 0) return -1;
	
	retS = Stat("/d/recursive_stat1/../recursive_stat2", &s);
	if(retS == 0) return -1;

	retS = Stat("/d/recursive_stat1/./recursive_stat2", &s);
	if(retS < 0) return -1;
	
	retS = Stat("/d/recursive_stat1/./recursive_stat1", &s);
	if(retS == 0) return -1;
	
	retS = Stat("/d/recursive_stat1/../recursive_stat1/recursive_stat2/recursive_stat3", &s);
	if(retS < 0) return -1;
	
	retS = Stat("/d/recursive_stat1/../recursive_stat2/recursive_stat2/recursive_stat3", &s);
	if(retS == 0) return -1;
	
	retS = Stat("/d/recursive_stat1/./recursive_stat2/../recursive_stat2/recursive_stat3", &s);
	if(retS < 0) return -1;
	
	retS = Stat("/d/recursive_stat1/./recursive_stat1../recursive_stat2/recursive_stat3", &s);
	if(retS == 0) return -1;
	
	(void)Delete("/d/recursive_stat1/recursive_stat2/recursive_stat3/recursive_stat4");
	(void)Delete("/d/recursive_stat1/recursive_stat2/recursive_stat3");
	(void)Delete("/d/recursive_stat1/recursive_stat2");
	(void)Delete("/d/recursive_stat1");
	
	return 0;
	
}

int tReadEntry()
{
  int fd, retR, retC;
  struct VFS_Dir_Entry dirEntry;

  retC = Create_Directory("/d/basic11d");
  if (retC < 0) {
    Print("couldn't create basic11d: %d %s\n", retC, Get_Error_String(retC));
    return -1;
  }

  retC = Create_Directory("/d/basic11d/d1");
  if (retC < 0) {
    Print("couldn't create basic11d/d1: %d %s\n", retC, Get_Error_String(retC));
    return -1;
  }

  retC = Create_Directory("/d/basic11d/d2");
  if (retC < 0) {
    Print("couldn't create basic11d/d2: %d %s\n", retC, Get_Error_String(retC));
    return -1;
  }

  fd = Open("/d/basic11d/f1", O_CREATE);
  if (fd < 0) {
    Print("couldn't open basic11d/f1: %d %s\n", fd, Get_Error_String(fd));
    return -1;
  }

  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open_Directory("/d/basic11d");
  if (fd < 0) {
    Print("couldn't opendir basic11d: %d %s\n", fd, Get_Error_String(fd));
    return -1;
  }

  retR = Read_Entry(fd, &dirEntry);

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
  
  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  fd = Open_Directory("/d/basic11d");
  if (fd < 0)
    return -1;

  // no  retR = Seek(fd, 2);
  // no if (retR < 0)
  // no   return -1;

  // no retR = Read_Entry(fd, &dirEntry);

  // no if ((retR < 0) ||
  // no     (strncmp(dirEntry.name, "f1", 2) != 0) ||
  // no      (dirEntry.stats.isDirectory))
  // no   return -1;
  
  if(Close(fd) < 0) { Print("failed to close"); return -1; }

  (void)Delete("/d/basic11d/d1");
  (void)Delete("/d/basic11d/d2");
  (void)Delete("/d/basic11d/f1");

  (void)Delete("/d/basic11d");

  return 1;
}

int tWriteReread(int howManyKBs, char const * fileName, bool reverseWrite)
{
  int fd, retW, retR, retS;
  int i, j;
  char buffer[100], buffer2[100];
  int ret = 1;
  struct VFS_File_Stat s;

  
  if(howManyKBs > ((blocksize / 4) + 4) * blocksize / 1024) {
    howManyKBs = ((blocksize / 4) + 4) * blocksize / 1024;
    Print("capping reread test at %d KB\n", howManyKBs);
  }

  for (j = 0; j < 100; j++ )
    buffer[j] = j;

  fd = Open(fileName, O_CREATE|O_WRITE);
  if (fd < 0)
    return -1;

  if(reverseWrite) {
    for (i = (howManyKBs * 10)-1; i>=0; i-- )
    {
      buffer[0] = i%256;
      retS = Seek(fd,i*100);
      if(retS != 0) { Print("seek failed\n"); ret =-1; break; }
      retW = Write( fd, buffer, 100);
      if (retW != 100) { Print("write %d failed\n", i); ret = -1; break; }

      retS = FStat(fd, &s); 
      if(retS != 0) { Print("fstat failed\n"); ret =-1; break; }
      if(s.size != howManyKBs * 1000) { Print("fstat size check failed: %d != %d\n", s.size, howManyKBs * 1000); ret =-1; break; }
      if(s.isDirectory) { Print("fstat isDirectory check failed\n"); ret =-1; break; }

      if (i%50 == 0)
        Print(" %d", i);
    }
  } else {
  for (i = 0; i < (howManyKBs * 10); i++ )
    {
      buffer[0] = i%256;
      retW = Write( fd, buffer, 100);
      if (retW != 100)
	{
	  ret = -1;
	  break;
	}
      retS = FStat(fd, &s); 
      if(retS != 0) { Print("fstat failed\n"); ret =-1; break; }
      if(s.size != (i+1) * 100) { Print("fstat size check failed: %d != %d\n", s.size, (i+1)*100); ret =-1; break; }
      if(s.isDirectory) { Print("fstat isDirectory check failed\n"); ret =-1; break; }
      
      
      if (i%50 == 0)
	Print(" %d", i);
    }
  }

  if(Close(fd) < 0) { Print("failed to close"); return -1; }
  
  retS = Stat(fileName, &s); 
  if(retS != 0) { Print("stat failed\n"); ret =-1;  }
  if(s.size != howManyKBs * 1000) { Print("stat size check failed: %d != %d\n", s.size, howManyKBs * 1000); ret =-1; }
  if(s.isDirectory) { Print("stat isDirectory check failed\n"); ret =-1;  }
  

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
        Print("read %d failed", i);
	      ret = -1;
	      break;
	    }

    if((unsigned char)buffer2[0] != (unsigned char)(i%256)) {
      Print("mismatched ident %d != %d", buffer2[0], i%256);
      ret = -1;
      break;
    }
	  for(j = 1; j < 100; j++)
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

      if(Close(fd) < 0) { Print("failed to close"); return -1; }
      (void)Delete(fileName);
    }

  return ret;
}

int t10KWriteReread()
{
  return tWriteReread(10, "/d/file_10k", false);
}

int t100KWriteReread()
{
  return tWriteReread(100, "/d/file_100k", false);
}
int t10KWriteRereadR()
{
  return tWriteReread(10, "/d/file_10kr", true);
}

int t100KWriteRereadR()
{
  return tWriteReread(100, "/d/file_100kr", true);
}

int tBigDir() {
  int retC, retD, fi, retS;
  char fname[50];
  struct VFS_File_Stat s;

  retC = Create_Directory("/d/bigdir");
  if(retC != 0) { Print("couldn't create /d/bigdir: %d\n", retC); return -1; }  
  
  for(fi=0; fi<100; fi++) {
    int fd;
    
    snprintf(fname, 50, "/d/bigdir/%04dabcdefghijklmnopqrstuvwxyz%04d", fi, fi);
    Print((fi % 25 == 0) ? ":" : ".");
    fd = Open(fname, O_WRITE|O_CREATE);
    if(fd < 0) { Print("bad open/creat at %d\n", fi); return -1; }
    if(Close(fd) < 0) { Print("failed to close"); return -1; }
    retS = Stat(fname, &s);
    if(retS < 0) { Print("bad stat at %d\n", fi); return -1; }
  }
  
  snprintf(fname, 50, "/d/bigdir/%04dabcdefghijklmnopqrstuvwxyz%04d", fi, fi);
  retS = Stat(fname, &s);
  if(retS == 0) { Print("bad extra stat at %d\n", fi);return -1; }
  
  for(fi=0; fi<100; fi++) {    
    snprintf(fname, 50, "/d/bigdir/%04dabcdefghijklmnopqrstuvwxyz%04d", fi, fi);
    Print((fi % 25 == 0) ? ":" : ".");
    (void)Delete(fname);
    retS = Stat(fname, &s);
    if(retS == 0) { return -1; }
  } 
  
  retD = Delete("/d/bigdir");
  if (retD != 0) { Print("failed to remove /d/bigdir: %d", retD); return retD; }

  return 0;
}
  

  

int tExhaustDisk() {
  /* (indirect + direct) * blocksize div by B/KB */
  int max_file_size_k = ((blocksize / 4) + 4) * blocksize / 1024;
  int files_needed_to_fill_disk = disksize_mb * 1024 * 63 / 64 / max_file_size_k;
  int files_needed_to_use_inodes = disksize_mb * 1024 * 1024 / 64 / sizeof(struct gfs2_inode);
  int i, fi, retC, retW, retD;
  char writeme[512];
  char fname[50];
  char dirname[25] = "";
  int repetition;

  
  Print("need %d files to fill disk, %d to use all inodes\n", files_needed_to_fill_disk, files_needed_to_use_inodes);
  
  if(files_needed_to_fill_disk > files_needed_to_use_inodes) {
    files_needed_to_fill_disk = files_needed_to_use_inodes;
  }
#define MARK  Print("%d:", __LINE__);
  retC = Create_Directory("/d/exhaust");
  if(retC != 0) { Print("couldn't create /d/exhaust\n"); return -1; }
  
  for(i=0; i<sizeof(writeme); i++) {
    writeme[i] = i%256;
  }
  
  for(repetition=0; repetition<3; repetition++) { 
    int files_worked_on;
    retW = 0;
    for(fi=0; retW >= 0 && fi<files_needed_to_fill_disk; fi++) {
      int fd;
      unsigned long b;
      
      if(fi%100 == 0) {
        snprintf(dirname, 25, "/d/exhaust/%d", fi/100);
        retC = Create_Directory(dirname);
        Print("%d/%d", fi, files_needed_to_fill_disk);
      }
      
      snprintf(fname, 50, "%s/%d", dirname, fi);
      Print((fi % 25 == 0) ? ":" : ".");
      
      fd = Open(fname, O_WRITE|O_CREATE);
      if(fd<0) { Print("failed to open %s\n", fname); return -1; }
      
      for(b=0; b<max_file_size_k*1024 - 100; b+= retW) {
        retW = Write(fd, writeme, 100);
        if(retW < 0) { Print("write %s %lu failed: %d", fname, b, retW); break; }
      }
      
      if(Close(fd) < 0) { Print("failed to close"); return -1; }
    }
    files_worked_on = fi;
    for( ; fi >=0; fi--) {
      snprintf(dirname, 25, "/d/exhaust/%d", fi/100);
      snprintf(fname, 50, "%s/%d", dirname, fi);
      (void)Delete(fname);
    }
    for(fi=0;fi<files_worked_on; fi+=100) {
      snprintf(dirname, 25, "/d/exhaust/%d", fi/100);
      if(Delete(dirname) < 0) { Print("couldnt remove %s", dirname); return -1; }
    }

  }  
  
  retD = Delete("/d/exhaust");
  if (retD != 0) { Print("failed to remove /d/exhaust: %d", retD); return retD; }
  
  return 0;
}

int tSync() {
  return Sync();
}

int tClean() {
  struct VFS_Dir_Entry dirEntry;
  int fd, retR, i;
  fd = Open_Directory("/d");
  for(i=0; (retR = Read_Entry(fd, &dirEntry)) == 0 && dirEntry.name[0] == '.'; i++);
  if(dirEntry.name[0] != '.') {
    return -1; /* not empty */
  }
  if(i>2) {
    return -1; /* many dotfiles */
  }
  if(retR != VFS_NO_MORE_DIR_ENTRIES) {
    return -1; /* failed out */
  }
  return 1;
}

int main(int argc, char **argv)
{


  int score = 0; int totalTests = 0; int successfulTests = 0;
  
  if(argc > 2) {
    disksize_mb = atoi(argv[1]);
    blocksize = atoi(argv[2]);
    if(blocksize != 512 && blocksize != 1024 && blocksize != 4096) {
      Print("illegal blocksize %s", argv[2]);
      Print("usage p5test [disksize_mb blocksize_b]");
    }
    if(disksize_mb == 0 || disksize_mb > 32) {
      Print("illegal disksize %s", argv[1]);
      Print("usage p5test [disksize_mb blocksize_b]");
    }
  } else if(argc > 1) {
    Print("will fail as soon as a test fails.");
    fail_immediately = true;
  }

  // 0
  // x doTestOrDie( "Format", ttestFormat, 1,  &score, &totalTests, &successfulTests);
  // 1
  doTestOrDie( "Mount", ttestMount, 1,  &score, &totalTests, &successfulTests);
  // 2
  doTest( "Open-Inexistent File", tOpenInexistentFile, 1,  &score, &totalTests, &successfulTests);
  // 3
  doTestOrDie( "Creat", tCreat, 2,  &score, &totalTests, &successfulTests);
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
  doTest( "Stat-File", tStatFile, 2,  &score, &totalTests, &successfulTests);
  // 20
  doTest( "Stat-Directory", tStatDirectory, 2,  &score, &totalTests, &successfulTests);

	doTest("Recursive Stat", tRecursiveStat, 4, &score, &totalTests, &successfulTests);
  // 21
  doTest( "Delete Empty Directory", tDeleteEmptyDirectory, 3,  &score, &totalTests, &successfulTests);
  // 22
  doTest( "Delete Non-Empty Directory", tDeleteNonEmptyDirectory, 2,  &score, &totalTests, &successfulTests);
  
  doTest( "Sync", tSync, 1, &score, &totalTests, &successfulTests);

  // 23
  doTest( "10k Write/Reread", t10KWriteReread, 4,  &score, &totalTests, &successfulTests);
  // 24
  doTest( "100k Write/Reread", t100KWriteReread, 6,  &score, &totalTests, &successfulTests);
  // 25
  // 23
  doTest( "10k Write/Reread Reverse", t10KWriteRereadR, 4,  &score, &totalTests, &successfulTests);
  // 24
  doTest( "100k Write/Reread Reverse", t100KWriteRereadR, 6,  &score, &totalTests, &successfulTests);
  // not this one doTest( "Read Entry", tReadEntry, 4,  &score, &totalTests, &successfulTests);

  doTest( "Big Directory", tBigDir, 7,  &score, &totalTests, &successfulTests);

  doTest( "Exhaust Disk", tExhaustDisk, 7,  &score, &totalTests, &successfulTests);

  doTest( "House Is Clean", tClean, 2,  &score, &totalTests, &successfulTests);

  Print ("********************************************\n");
  Print ("Tests attempted: %d. Passed: %d. Failed: %d\n", totalTests, successfulTests, (totalTests-successfulTests) );
  Print ("SCORE: %d\n", score);

  return 0;
}


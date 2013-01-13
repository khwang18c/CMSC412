#include <stdio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <geekos/gfs2.h>

#include <geekos/bitset.h>

/* geekos will read and write only in units of 512-byte sectors. */
/* you probably don't need this defined here, but it's there in case 
   you do */
#define SECTOR_SIZE 512

int main(int argc, const char *argv[]) {
  /* argv[1]: output file name */
  /* argv[2]: block size: 512, 1024, 4096 */
  /* argv[3]: number of blocks */
  /* argv[4...]: files to include, optional (not tested) */
  assert(argc>0); /* compiler warning appeasement */
  assert(argv != NULL); /* compiler warning appeasement */
  

  exit(EXIT_FAILURE); /* replace me with EXIT_SUCCESS */
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "libDisk.h"
#include "tinyFS_errno.h"
#include "tinyFS.h"

/* Opens a regular UNIX file and designates the first nBytes of it as
 * space for an emulated disk.
 *
 * If nBytes is not exactly a multiple of BLOCKSIZE then the disk size
 * will be the closest multiple of BLOCKSIZE that is lower than nByte
 * (but greater than 0).
 * If nBytes is less than BLOCKSIZE failure should be returned.
 * If nBytes > BLOCKSIZE and there is already a file by the given
 * filename, that file’s content may be overwritten.
 * If nBytes is 0, an existing disk is opened, and the content must
 * not be overwritten in this function.
 */
int openDisk(char *filename, int nBytes){
  int i, disk;
  int flags = O_RDWR;
  mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
  char * empty;

  empty = calloc(256, sizeof(char));
  if(!empty){
    perror("openDisk calloc");
    exit(EXIT_FAILURE);
  }
  
  /*if nBytes is a non-zero number smaller than blocksize, fail*/
  if((nBytes) && (nBytes < BLOCKSIZE)){
    return INVALID_SIZE;
  }else if(nBytes){
    /*if nBytes is not zero add truncation onto the list of flags*/
    flags |= O_TRUNC|O_CREAT;
  }
  disk=open(filename, flags, mode);
  if((disk == -1)){
    perror("Disk Open error");
    return DISK_OPEN_ERR;
  }

  if(nBytes){
    for(i = 0; i < (nBytes / BLOCKSIZE); i++){
      int check = write(disk, empty, BLOCKSIZE);
      if(check != BLOCKSIZE){
	write(disk, empty, BLOCKSIZE-check);
      }else if(check == -1){
	perror("Write error");
	return DISK_WRITE_ERR;
      }
    }
  }
  if(lseek(disk, 0, SEEK_SET)<0){
    perror("Disk error");
    return DISK_ERR;
  }
  return disk;
}

int closeDisk(int disk){
  if(close(disk)!=0){
    perror("Disk close Error");
    return DISK_CLOSE_ERR;
  }else{
    return TFS_SUCCESS;
  }
}


/* Read an entire block of BLOCKSIZE bytes from an open disk and
 * copies the result into a local buffer (must be at least of
 * BLOCKSIZE bytes).
 * bNum is a logical block number. The translation from logical to
 * physical block is as follows:
 * If bNum=0 is the very first block of the disk.
 * If bNum=1 is BLOCKSIZE bytes into the disk.
 * If bNum=n is n*BLOCKSIZE bytes into the disk. On success, it
 *
 * returns 0 on success or -1 or smaller on error.
 */
int readBlock(int disk, int bNum, void *block){
  int offset = bNum * BLOCKSIZE;
  if(lseek(disk, offset, SEEK_SET)<0){
    perror("Disk Error");
    return DISK_ERR;
  }
  if(read(disk, block, BLOCKSIZE)<0){
    perror("Disk Read");
    return DISK_READ_ERR;
  }
  return TFS_SUCCESS;
}

/* Takes disk number ‘disk’ and logical block number ‘bNum’ and writes
 * the content of the buffer ‘block’ to that location.
 * ‘block’ must be integral with BLOCKSIZE.
 * returns 0 on success of -1 or smaller on error.
 */
int writeBlock(int disk, int bNum, void *block){
  int offset = bNum * BLOCKSIZE;
  if(lseek(disk, offset, SEEK_SET)<0){
    perror("Disk Error");
    return DISK_ERR;
  }
  /*If write writes less than blocksize, then underlying issues
    are afoot.*/
  if(write(disk, block, BLOCKSIZE) < BLOCKSIZE){
    perror("Disk Write");
    return DISK_WRITE_ERR;
  }
  return TFS_SUCCESS;
}

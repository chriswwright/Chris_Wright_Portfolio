#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyFS.h"
#include "libTinyFS.h"
#include "tinyFS_errno.h"

/* simple helper function to fill Buffer with as many inPhrase strings as possible before reaching size */
int fillBufferWithPhrase (char *inPhrase, char *Buffer, int size){
  int index = 0, i;
  if (!inPhrase || !Buffer || size <= 0 || size < strlen (inPhrase))
    return -1;

  while (index < size)
    {
      for (i = 0; inPhrase[i] != '\0' && (i + index < size); i++)
	Buffer[i + index] = inPhrase[i];
      index += i;
    }
  Buffer[size - 1] = '\0';	/* explicit null termination */
  return 0;
}

int main(){
  char readBuffer;
  char *afileContent, *bfileContent;
  int afileSize = 500;
  int bfileSize = 10 * BLOCKSIZE;

  char phrase1[] = "(a) file, this should not break the system. ";
  char phrase2[] = "This is (b) file. ";

  int status;
  fileDescriptor aFD, bFD;
  time_t creation;
  time_t last_read;
  time_t last_write;
  
  /*try to mount the disk*/

  if(tfs_mount(DEFAULT_DISK_NAME) < 0){
    printf("Mount fails:\n");
    tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE);
    if(tfs_mount(DEFAULT_DISK_NAME) < 0){
      perror("failed to open disk");
      return EXIT_FAILURE;
    }
  }

  /*Create afile*/
  afileContent = (char*) malloc(afileSize * sizeof(char));
  if(fillBufferWithPhrase(phrase1, afileContent, afileSize)<0){
    perror("failed");
    return EXIT_FAILURE;
  }

  /*Create bfile*/
  bfileContent = (char*) malloc(bfileSize * sizeof(char));
  if(fillBufferWithPhrase(phrase2, bfileContent, bfileSize)<0){
    perror("failed");
    return EXIT_FAILURE;
  }
  

  printf("afile content: %s\n\nbfile content: %s\n\nReady to store in TinyFS\n",
	 afileContent, bfileContent);
  
  aFD = tfs_openFile("afile");
  if(aFD<0){
    perror("tfs_openFile failed on afile");
  }
  
  /*If the file does not exist, write to it, else read from it*/
  if(tfs_readByte(aFD, &readBuffer) <0){
    if(tfs_writeFile(aFD, afileContent, afileSize) < 0){
      perror("tfs_writeFile failed");
    }else{
      printf("successfully written to afile\n");
      printf("setting afile to read only\n");
      tfs_makeRO("afile");
	     
    }
  }else{
    printf("\n*** reading afile from TinyFS: \n%c", readBuffer);
    while(tfs_readByte(aFD, &readBuffer)>=0){
      printf("%c", readBuffer);
    }
    /*read the time information*/
    creation = tfs_readFileInfo(aFD);
    if(creation<0){
      TFSERR_OUT((int)creation);
    }
    last_read = tfs_lastReadTime(aFD);
    if(last_read<0){
      TFSERR_OUT((int)last_read);
    }
    last_write = tfs_lastWriteTime(aFD);
    if(last_write<0){
      TFSERR_OUT((int)last_write);
    }
    printf("\n\nafile Metadata:\n  creation: %s", ctime(&creation));
    printf(" last read: %s", ctime(&last_read));
    printf("last write: %s", ctime(&last_write));

    status = tfs_deleteFile(aFD);
    if(status<0){
      if(status == TFS_ILLEGAL_WRITE_ERR){
	printf("\nafile cannot be deleted because it is read only");
	printf("\nChanging afile to RW and Writing 200 '!' characters to afile\n");
	status = tfs_seek(aFD, 400);
	if(status < 0){
	  TFSERR_OUT(status);
	}
	tfs_makeRW("afile");
	int i;
	for(i = 0; i<200; i++){
	  tfs_writeByte(aFD, '!');
	}
      }else{
	TFSERR_OUT(status);
      }
    }else{
      printf("deleting afile\n");
    }
  }

  bFD = tfs_openFile("bfile");
  
  if(tfs_readByte(bFD, &readBuffer) <0){
    if(tfs_writeFile(bFD, bfileContent, bfileSize) < 0){
      perror("tfs_writeFile failed");
    }else{
      printf("successfully written to bfile\n");
      printf("setting bfile to read only\n");
      tfs_makeRO("bfile");
    }
  }else{
        
    printf("\n*** reading bfile from TinyFS: \n%c", readBuffer);
    while(tfs_readByte(bFD, &readBuffer)>=0){
      printf("%c", readBuffer);
    }
    /*read the time information*/
    creation = tfs_readFileInfo(bFD);
    if(creation<0){
      TFSERR_OUT((int)creation);
    }
    last_read = tfs_lastReadTime(bFD);
    if(last_read<0){
      TFSERR_OUT((int)last_read);
    }
    last_write = tfs_lastWriteTime(bFD);
    if(last_write<0){
      TFSERR_OUT((int)last_write);
    }
    printf("\n\nbfile Metadata:\n  creation: %s", ctime(&creation));
    printf(" last read: %s", ctime(&last_read));
    printf("last write: %s", ctime(&last_write));

    tfs_seek(bFD, 0);
    status = tfs_writeByte(bFD, '5');
    if(status< 0){
      if(status == TFS_ILLEGAL_WRITE_ERR){
	printf("\nbfile cannot be written to because it is read only");
	printf("\nChanging bfile to RW, and writing 256 '5' to bfile");
	tfs_makeRW("bfile");
	int i;
	for(i = 0; i<256; i++){
	  tfs_writeByte(bFD, '5');
	}
      }else{
	TFSERR_OUT(status);
      }
    }else{
      printf("deleting bfile\n");
      tfs_deleteFile(bFD);
    }
  }

  free(afileContent);
  free(bfileContent);
  if(tfs_unmount() < 0){
    perror("tfs_unmount_failed");
  }

  printf("\nEnd of demo\n\n");
  return 0;
}

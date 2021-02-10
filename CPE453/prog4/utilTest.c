#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tinyFS.h"
#include "tinyFS_errno.h"
#include "libDisk.h"
#include "utilTinyFS.h"

#define NUM_BLOCKS 20

int main(){
  int i;
  int check = 0;
  int disk;
  char * diskname = "utiltest.dsk";
  uint8_t * buffer; /*Holds one block of information*/
  uint8_t * bitmap; /*Holds a numeric representation of a bitmap*/
  char * filename; /*Holds a sample filename for testing*/
  
  disk = openDisk(diskname, NUM_BLOCKS * BLOCKSIZE);
  if(disk < 0){
    printf("Open disk failed to create a disk with %i. exit.\n", disk);
    exit(EXIT_FAILURE);
  }

  printf("testing tfs_fileNameValid()\n");
  filename = calloc(MAX_PATH_LEN + 1, sizeof(char));
  filename = strncpy(filename, "abc456HI9", 10);
  check = tfs_fileNameValid(filename);
  if(check != TFS_FILENAME_TOO_LONG){
    printf("  tfs_fileNameValid() failed to find string %s invalid\n", filename);
    exit(EXIT_FAILURE);
  }
  filename[8] = 0x00;
  check = tfs_fileNameValid(filename);
  if(check == TFS_FILENAME_TOO_LONG){
    printf("  tfs_fileNameValid() failed to find string %s valid\n", filename);
    exit(EXIT_FAILURE);
  }
  filename=strncpy(filename, "abc/def", 8);
  check = tfs_fileNameValid(filename);
  if(!(check < 0)){
    printf("  tfs_fileNameValid() failed to find string %s as invalid\n", filename);
    exit(EXIT_FAILURE);
  }
  filename = strncpy(filename, "/a/b/c/d/e/f/g", 15);
  check = tfs_fileNameValid(filename);
  if(!(check < 0)){
    printf("  tfs_fileNameValid() failed to find string %s as invalid\n", filename);
    exit(EXIT_FAILURE);
  }/*
  filename = strncpy(filename, "/abc/def/gh?", 13);
  check = tfs_fileNameValid(filename);
  if(!(check < 0)){
    printf("  tfs_fileNameValid() failed to find string %s as ivalid\n", filename);
    exit(EXIT_FAILURE);
  }
  filename = strncpy(filename, "/a/b/c/d/e/f/g/h/i/j/k", 23);
  check = tfs_fileNameValid(filename);
  if(!(check < 0)){
    printf("  tfs_fileNameValid() failed to find string %s as ivalid\n", filename);
    exit(EXIT_FAILURE);
  }*/
  filename = strncpy(filename, "/abcdefgh/abcdefgh/abcdefgh/abcdefgh/abcdefgh/abcdefgh/abcdefgh", MAX_PATH_LEN + 1);
  /*check = tfs_fileNameValid(filename);
  if(check < 0){
    printf("  %i: tfs_fileNameValid() failed to find string %s as valid\n", check, filename);
    exit(EXIT_FAILURE);
  }*/


  printf("Testing block creating and writing\n");						   
  
  buffer = calloc(BLOCKSIZE, sizeof(char));
  //filename = strncpy(filename, "12345678", 9);
  filename[9] = 0x00;
  
  printf("  Writing a superblock\n");
  tfs_mksuper(buffer, BLOCKSIZE, NUM_BLOCKS);
  writeBlock(disk, 0, buffer);

  printf("  Writing a inode\n");
  tfs_mkinode(buffer, BLOCKSIZE, filename, 0, FILE_READ_WRITE);
  writeBlock(disk, 1, buffer);

  printf("  Writing an extent block\n");  
  tfs_mkextent(buffer, BLOCKSIZE, (uint8_t *)filename, MAX_PATH_LEN);
  writeBlock(disk, 2, buffer);

  printf("  Writing an empty block\n");
  tfs_mkempty(buffer, BLOCKSIZE);
  writeBlock(disk, 3, buffer);

  printf("sample blocks written\n");

  readBlock(disk, 0, buffer);


  printf("Testing bitmap manipulation\n");
  printf("  Bitmap starting value:");
  for(i = 0; i < 32; i++){
    printf("%X, ", (buffer + SUPER_FREE_BLOCKS)[i]);
  }
  printf("\n");
	 
  bitmap = calloc(NUM_BLOCKS, sizeof(char));
  check = readBitmap(buffer + SUPER_FREE_BLOCKS, bitmap, NUM_BLOCKS);
  if(check < 0){
    printf("readBitmap failed\n");
    exit(EXIT_FAILURE);
  }

  printf("  free block bitmap contents:");  
  for(i = 0; i < NUM_BLOCKS; i++){
    printf("%i, ", bitmap[i]);
  }
  printf("\n");

  for(i = NUM_BLOCKS / 2; i<NUM_BLOCKS; i++){
    modifyBitmap(buffer + SUPER_FREE_BLOCKS, i);
  }

  printf("  Bitmap ending value:");
  for(i = 0; i < 32; i++){
    printf("%X, ", ((uint8_t)(buffer + SUPER_FREE_BLOCKS)[i]));
  }
  printf("\n");
  
  check = readBitmap(buffer + SUPER_FREE_BLOCKS, bitmap, NUM_BLOCKS);
  if(check < 0){
    printf("readBitmap failed\n");
    exit(EXIT_FAILURE);
  }

  printf("  free block bitmap contents:");
  for(i = 0; i < NUM_BLOCKS; i++){
    printf("%i, ", bitmap[i]);
  }
  printf("\nFinished\n");

  return 0;
}

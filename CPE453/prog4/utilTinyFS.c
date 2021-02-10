#include <ctype.h>
#include <stdint.h>

#include "tinyFS_errno.h"
#include "utilTinyFS.h"
#include "tinyFS.h"

/* Takes a pointer to the start of a 32 byte (256 bit) long bitmap
 * (bitmap), and populates a buffer (buffer) of size 'size' bytes to a
 * list of numbers which represent which bits were set as 1 in the
 * bitmap. If the buffer is too small, return an error.
 */
int readBitmap(uint8_t * bitmap, uint8_t *buffer, int size){
  int i, j, buf_offset;
  uint8_t mask[] = {0x80, 0x40, 0x20, 0x10,
                 0x08, 0x04, 0x02, 0x01};

  /*Reset the destination buffer*/
  for(i = 0; i < size; i++){
    buffer[i] = 0;
  }
  buf_offset = 0;
  /*For each byte of the bitmap*/
  for(i = 0; i < 32; i++){
    /*for each bit of the byte*/
    for(j = 0; j < 8; j++){
      /* if the bit is 1 add a numerical representation of the set bit
         to buffer*/
      if(mask[j] & bitmap[i]){
	/*If we're beyond the size of buffer, return an error*/
	if(buf_offset > size){
	  return BITMAP_BUF_SIZE;
	}
	buffer[buf_offset] = (i*8) + j;
	buf_offset ++;
      }
    }
  }
  return 0;
}

/* Flips the bit at position pos in the 256 bit long bitmap stored in
 * bitmap
 */
int  modifyBitmap(uint8_t * bitmap, int pos){
  uint8_t mask[] = {0x80, 0x40, 0x20, 0x10,
                 0x08, 0x04, 0x02, 0x01};
  
  bitmap[pos / 8] ^= mask[pos % 8];
  return 0;
}

/* populates a buffer of size 'size' with the information pertaining
 * to a tinyFS supernode, in the format as follows
 * 
 * Byte 0: Block type - 0x01
 * Byte 1: Magic number - 0x44
 * Byte 2: link to root directory (for future use) - 0x01;
 * Byte 3: Empty
 * Byte 4: Disk size (in blocks) 1->255
 * Byte 5->36: Free block bitmap
 *
 * The free block bitmap represents all blocks avalible in the
 * filesystem. byte 5 represents blocks 0->7, byte 6 represents block
 * 8->15 etc. Use readBitmap() to translate this value to a list of
 * uint8_t
 *
 * returns 0 on success, <0 on failure
 */
int tfs_mksuper(uint8_t * buffer, int size, int numBlocks){
  int i;
  uint8_t mask[] = {0x80, 0x40, 0x20, 0x10,
                 0x08, 0x04, 0x02, 0x01};
  
  buffer[BLOCK_TYPE] = 0x01;
  buffer[MAGIC_NUM_LOC] = 0x44;
  buffer[BLOCK_LINK] = 0x01;
  buffer[EMPTY_BYTE] = 0x00;
  buffer[SUPER_DISK_SIZE] = (uint8_t)numBlocks;

  for(i = 1; i < numBlocks; i++){
    buffer[SUPER_FREE_BLOCKS + (i/8)] |= mask[i%8];
  }

  for(i = 37; i < size; i++){
    buffer[i] = 0x00;
  }
  return 0;
}

/* Populates a buffer of size 'size' bytes with the information
 * pertaining to a tinyFS inode, in the format that follows:
 *
 * Byte 0: Block type - 0x02
 * Byte 1: Magic number - 0x44
 * Byte 2: link
 * Byte 3: Empty
 * Byte 4: inode type: FILE_INODE for file inode,
 *                     DIRECTORY_INODE for direcory inode
 * Bytes 5->12: Filename: 8 alphanumeric characters (NON NULL TERMINATED)
 * Bytes 13-14: Filesize in Bytes: A value between 0 and 65536 bytes
 * Byte 15: Filesize in blocks
 * Byte 16-23: Time the file was created (stored as a time_t)
 * byte 24-31: Time of the file's last read (stored as a time_t)
 * byte 32-39: time of the file's last write
 * Bytes 40->255: For file inode - pointers to extent blocks
 *                For directory inode - pointers to children inodes
 */
int tfs_mkinode(uint8_t * buffer, int size, char * name, time_t creation, int type){
  int i;

  buffer[BLOCK_TYPE] = 0x02;
  buffer[MAGIC_NUM_LOC] = 0x44;
  buffer[BLOCK_LINK] = 0x00;
  buffer[EMPTY_BYTE] = 0x00;

  buffer[INODE_TYPE] = (uint8_t)type;
  for(i = 0; i < 8; i++){
    if(name[i] == 0x00){
      break;
    }
    buffer[INODE_FILENAME+i] = name[i];
  }

  time_t * creat_time = (time_t *)(buffer+INODE_CREATION_TIME);
  *creat_time = creation;
  
  for(i=INODE_READ_TIME; i < size; i++){
    buffer[i] = 0;
  }
  return 0;
}

/* Populates a buffer of size 'size' bytes with the information
 * pertainint to a tinyFS data extent node, with the following tinyFS
 * header, and 'length' bytes of data from uint8_t * data
 *
 * Header:
 * Byte 0: Block type - 0x03
 * Byte 1: Magic number - 0x44
 * Byte 2: link
 * Byte 3: Empty
 * Byte 4->255 Data
 */
int tfs_mkextent(uint8_t * buffer, int size, uint8_t * data, int length){
  int i;

  /*if length is more than size - 4 (space for the header), return an
    error */
  if(length > (size - 4)){
    return -1;
  }
  
  buffer[BLOCK_TYPE] = 0x03;
  buffer[MAGIC_NUM_LOC] = 0x44;
  buffer[BLOCK_LINK] = 0x00;
  buffer[EMPTY_BYTE] = 0x00;
  
  for(i = 0; i < length; i++){
    buffer[i + EXTENT_DATA] = data[i];
  }
  
  return 0;
}


/* Modifies the provided buffer of size 'size' such that the first 4
 * bytes fulfils the format of an empty block:
 *
 * Header:
 * Byte 0: Block type - 0x04
 * Byte 1: Magic number - 0x44
 * Byte 2: link
 * Byte 3: Empty
 * Byte 4->255 Data
 */
int tfs_mkempty(uint8_t * buffer, int size){
  buffer[BLOCK_TYPE] = 0x04;
  buffer[MAGIC_NUM_LOC] = 0x44;
  buffer[BLOCK_LINK] = 0x00;
  buffer[EMPTY_BYTE] = 0x00;

  return 0;
}

/* Returns the next free block, given a buffer that represents the
 * superblock.
 *
 * Returns an error if there is no free block, or if the superNode is
 * invalid.
 */
int tfs_getFreeBlock(uint8_t * super){
  int i, j;
  uint8_t mask[] = {0x80, 0x40, 0x20, 0x10,
                 0x08, 0x04, 0x02, 0x01};
  uint8_t blkCnt = super[SUPER_DISK_SIZE];
  
  /*Check the magic number and block type*/
  if((super[MAGIC_NUM_LOC] != 0x44) || (super[BLOCK_TYPE] != 0x01)){
    return -1;
  }

  /*for every bit in the bitstream*/
  for(i = 0; i < 32; i++){
    for(j = 0; j < 8; j++){
      /*If past the last bit that represents a block, break*/
      if(((i * 8) + j) > blkCnt){
	break;
      }
      /*If the current bit is set, return that bit's position*/
      if(super[SUPER_FREE_BLOCKS + i] & mask[j]){
	return((i * 8) + j);
      }
    }
  }
  /*return the no free blocks error.*/
  return TFS_NO_FREE_BLOCKS;
}

/* checks if the 8 character string in the null terminated string
 * 'name' is a valid filename, consisting of 8 alphanumeric characters
 */
int tfs_fileNameValid(char * name){
  int i, nameLen, pathDepth;

  i = 0;
  nameLen = 0;
  pathDepth = 0;
  /*While there are characters to be read*/
  while(name[i] != 0x00){
    /*If the character is not alphanumeric*/
    if(!(isalnum(name[i]))){
      /*If the character is a path delimiter*/
      /*
      if(name[i] == '/'){
	pathDepth++;
	nameLen = 0;
      }else{
	return TFS_INVALID_FILENAME_CHAR;
      }*/
      return TFS_INVALID_FILENAME_CHAR;
    }else{
      nameLen++;
    }
    i++;
    
    /*If the length of this name is greater than 8, return an error*/
    if(nameLen > 8){
      return TFS_FILENAME_TOO_LONG;
    }
  }
  if(pathDepth > DIR_MAX_DEPTH){
    return TFS_PATH_TOO_DEEP;
  }
  
  return 0;
}

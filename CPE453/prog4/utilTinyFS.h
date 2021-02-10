#include <time.h>
#include <stdint.h>
#define BITMAP_SIZE 32

/*Defining header byte location macros*/
#define BLOCK_TYPE 0
#define MAGIC_NUM_LOC 1
#define BLOCK_LINK 2
#define EMPTY_BYTE 3

/*Defining superblock byte location macros*/
#define SUPER_DISK_SIZE 4
#define SUPER_FREE_BLOCKS 5
#define SUPER_FILE_COUNT 37
#define SUPER_INODE_BLOCKS 38
#define SUPER_EXCESS 71

/*Defining inode byte location macros*/
#define INODE_TYPE 4
#define INODE_FILENAME 5
#define INODE_FILESIZE 13
#define INODE_NUM_BLOCKS 15
#define INODE_CREATION_TIME 16
#define INODE_READ_TIME 24
#define INODE_WRITE_TIME 32
#define INODE_DATA 40

#define FILE_READ_WRITE 1
#define FILE_READ_ONLY 2

/*Defining extent byte location macros*/
#define EXTENT_DATA 4

/*defining directory max depth*/
#define DIR_MAX_DEPTH 7
#define MAX_PATH_LEN 9 * (DIR_MAX_DEPTH + 1)

typedef struct dynamic_t{
  int inode_num;
  int offset;
  int file_size;
  uint8_t* file_data;
}dynamic_list_entry;

/* Takes a pointer to the start of a 32 byte (256 bit) long bitmap
 * (bitmap), and populates a buffer (buffer) of size 'size' bytes to a
 * list of numbers which represent which bits were set as 1 in the
 * bitmap. If the buffer is too small, return an error.
 */
int readBitmap(uint8_t * bitmap, uint8_t * buffer, int size);

/* Flips the bit at position pos in the 256 bit long bitmap stored in
 * bitmap*/
int modifyBitmap(uint8_t * bitmap, int pos);

/* populates a buffer of size 'size' with the information pertaining
 * to a tinyFS supernode, in the format as follows
 * 
 * Byte 0: Block type - 0x01
 * Byte 1: Magic number - 0x44
 * Byte 2: Link (Not used)
 * Byte 3: Empty
 * Byte 4: Disk size (in blocks) 1->255
 * Byte 5->36: Free block bitmap
 * Byte 37: Number of files (total) on this disk
 * Byte 38->70: Inode bitmap
 *
 * The free block bitmap represents all blocks avalible in the
 * filesystem. byte 5 represents blocks 0->7, byte 6 represents block
 * 8->15 etc. Use readBitmap() to translate this value to a list of
 * uint8_t
 *
 * returns 0 on success, <0 on failure
 */
int tfs_mksuper(uint8_t * buffer, int size, int numBlocks);

/* Populates a buffer of size 'size' bytes with the information
 * pertaining to a tinyFS inode, in the format that follows:
 *
 * Byte 0: Block type - 0x02
 * Byte 1: Magic number - 0x44
 * Byte 2: link
 * Byte 3: Empty
 * Byte 4: inode type: FILE_READ_WRITE for r/w files
 *                     FILE_READ_ONLY for read only files
 * Bytes 5->12: Filename: 8 alphanumeric characters (NON NULL TERMINATED)
 * Bytes 13-14: Filesize in Bytes: A value between 0 and 65536 bytes
 * Byte 15: Filesize in blocks
 * Byte 16-23: Time the file was created (stored as a time_t)
 * byte 24-31: Time of the file's last read (stored as a time_t)
 * byte 32-39: time of the file's last write (stored as a time_t)
 * Bytes 40->255: For file inode - pointers to extent blocks
 *                For directory inode - pointers to children inodes
 */
int tfs_mkinode(uint8_t * buffer, int size, char * name, time_t creation, int type);

/* Populates a buffer of size 'size' bytes with the information
 * pertainint to a tinyFS data extent node, with the following tinyFS
 * header, and length bytes of data from char * data
 *
 * Header:
 * Byte 0: Block type - 0x03
 * Byte 1: Magic number - 0x44
 * Byte 2: link
 * Byte 3: Empty
 * Byte 4->255 Data
 */
int tfs_mkextent(uint8_t * buffer, int size, uint8_t * data, int length);

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
int tfs_mkempty(uint8_t * buffer, int size);

/* Returns the next free block, given a buffer that represents the
 * superblock.
 *
 * Returns an error if there is no free block, or if the superNode is
 * invalid.
 */
int tfs_getFreeBlock(uint8_t * super);

/* checks if the 8 character string in the null terminated string
   'name' is a valid filename.
 */
int tfs_fileNameValid(char * name);

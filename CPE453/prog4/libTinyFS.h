#include "tinyFS.h"
#include <time.h>
#define MAGIC_NUM 0x44
#define MAX_FILE_NAME_SIZE 8
#define MAX_FILE_SIZE 65535
/* Makes a blank TinyFS file system of size nBytes on the unix file
 * specified by ‘filename’.
 *
 * Uses the emulated disk library to open the specified unix file. On
 * success, format the file to be a mountable disk. Initialize all
 * blocks to empty blocks, setting magic numbers, initializing and
 * writing the superblock and inode(s) as needed.
 * 
 * returns 0 on success, <0 on error.
 */
int tfs_mkfs(char *filename, int nBytes);

/* Mounts a TinyFS file system located within ‘diskname’.
 *
 * Verifies the file system is the correct type. tinyFS only supports
 * one file system mounted at a time. Must return a specified
 * success/error code.
 *
 * returns 0 on success, <0 on errror
 */
int tfs_mount(char *diskname);
/* tfs_unmount(void) unmounts the currently mounted file system.
 */
int tfs_unmount(void); 

/* Creates or Opens a file for reading and writing on the currently
 * mounted file system.
 * 
 * Creates a dynamic resource table entry for the file, and returns a
 * file descriptor (integer) that can be used to reference this entry
 * while the filesystem is mounted.
 */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system resources, and removes
 * table entry
*/
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire
 * file’s content, to the file system.
 *
 * Previous content (if any) will be completely lost. Sets the file
 * pointer to 0 (the start of file) when done.
 *
 * Returns 0 on success, <0 on errror.
 */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk.
 */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the
 * current file pointer location and incrementing it by one upon
 * success.
 *
 * If the file pointer is already past the end of the file then
 * tfs_readByte() should return an error and not increment the file
 * pointer.
*/
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns
 * success/error codes.
*/
int tfs_seek(fileDescriptor FD, int offset);

int tfs_makeRO(char * name);

int tfs_makeRW(char * name);

int tfs_writeByte(fileDescriptor FD, unsigned int data);

time_t tfs_readFileInfo(fileDescriptor FD);

time_t tfs_lastReadTime(fileDescriptor FD);

time_t tfs_lastWriteTime(fileDescriptor FD);

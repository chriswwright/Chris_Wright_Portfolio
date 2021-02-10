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
int openDisk(char *filename, int nBytes);
int closeDisk(int disk);

/* Read an entire block of BLOCKSIZE bytes from an open disk and
 * copies the result into a local buffer (must be at least of
 * BLOCKSIZE bytes).
 * bNum is a logical block number. The translation from logical to
 * physical block is as follows:
 * If bNum=0 is the very first byte of the file.
 * If bNum=1 is BLOCKSIZE bytes into the disk.
 * If bNum=n is n*BLOCKSIZE bytes into the disk. On success, it
 *
 * returns 0 on success or -1 or smaller on error.
 */
int readBlock(int disk, int bNum, void *block);

/* Takes disk number ‘disk’ and logical block number ‘bNum’ and writes
 * the content of the buffer ‘block’ to that location.
 * ‘block’ must be integral with BLOCKSIZE.
 * returns 0 on success of -1 or smaller on error.
 */
int writeBlock(int disk, int bNum, void *block);

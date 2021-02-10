

#ifndef TFSERRORS
#include <stdio.h>
/* Note: numbers closer to 0 are (should be) more common:
 */

#define TFSERR_OUT(err) fprintf(stderr, "Tiny file system error: %i",err);

/*General or Catchall return values (0 >= error >= -5)*/
#define TFS_SUCCESS 0
#define TFS_FAIL -1

/*libdisk errors (-6 >= error >= -11)*/
#define INVALID_SIZE -6
#define DISK_OPEN_ERR -7
#define DISK_CLOSE_ERR -8
#define DISK_READ_ERR -9
#define DISK_WRITE_ERR -10
#define DISK_ERR -11

/*libTinyFS errors (-12 >= error >= -23)*/
#define TFS_INVALID_FS_ERR -12
#define TFS_FILE_IN_USE_ERR -13
#define TFS_OFFSET_PAST_FILE_EXTENT_ERR -14
#define TFS_END_OF_FILE_ERR -15
#define TFS_INVALID_FD_ERR -16
#define TFS_TOO_MANY_FILE_ERR -17
#define TFS_NON_EXISTANT_FILE_ERR -18
#define TFS_ILLEGAL_WRITE_ERR -19
#define TFS_BLOCK_ADDRESS_LIMIT_ERR -20
#define TFS_INSUFFICIENT_FREE_BLOCK_ERR -21
#define TFS_NEGATIVE_OFFSET_ERR -22
#define TFS_INVALID_WRITE_SIZE -23
/*utilTinyFS errors (-24 >= error)*/
#define BITMAP_BUF_SIZE -24
#define TFS_NO_FREE_BLOCKS -25
#define TFS_INVALID_FILENAME_CHAR -26
#define TFS_FILENAME_TOO_LONG -27
#define TFS_PATH_TOO_DEEP -28

#endif /*TFS errors*/

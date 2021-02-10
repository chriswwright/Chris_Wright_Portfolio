#include "libDisk.h"
#include "libTinyFS.h"
#include "utilTinyFS.h"
#include "tinyFS_errno.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

static int m_disk = -1;
static struct dynamic_t ** dynamic_file_list;
static int dynamic_table_maximum_size = 0;
static int block_count = 0;
static int file_limit = 0;

int tfs_mkfs(char *filename, int nBytes){
    block_count = nBytes/BLOCKSIZE;
    uint8_t* block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));

    //initalize disk for FS
    int disk = openDisk(filename, nBytes);
    if(disk < 0){
        return disk;
    }

    //populate super block for FS
    int status = tfs_mksuper(block_buffer, BLOCKSIZE, block_count);
    if(status < 0){
        return status;
    }
    //write super block to FS
    status = writeBlock(disk, 0, block_buffer);
    if(status < 0){
        return status;
    }

    //zeroing buffer
    memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));

    //populate empty block for FS
    status = tfs_mkempty(block_buffer, BLOCKSIZE);
    if(status < 0){
        return status;
    }

    //Write empty blocks to FS
    for(int i = 1; i < block_count; i++){
        status = writeBlock(disk, i, block_buffer);
        if(status < 0){
            return status;
        }
    }

    //Close FS after writing
    status = closeDisk(disk);
        if(status < 0){
            return status;
        }
    return status;
}

int tfs_mount(char *diskname){
    uint8_t* block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* bitmap_buffer = (uint8_t*)calloc(BLOCKSIZE+1, sizeof(uint8_t));
    //Attempt to open FS
    int status = openDisk(diskname, 0);
    if(status < 0){
        return status;
    }
 
    int disk = status;
    //Attempt to read super block to buffer
    status = readBlock(disk, 0, super_block);
    if(status < 0){
        return status;
    }

    //Testing super block validity
    if(super_block[BLOCK_TYPE] != 0x01){
        return TFS_INVALID_FS_ERR;
    }
    if(super_block[MAGIC_NUM_LOC] != MAGIC_NUM){
        return TFS_INVALID_FS_ERR;
    }
    status = readBitmap(super_block + SUPER_INODE_BLOCKS, bitmap_buffer, BLOCKSIZE*sizeof(char));
    if(status < 0){
        return status;
    }
    if(super_block[SUPER_FILE_COUNT] != strlen((char*)bitmap_buffer)){
        return TFS_INVALID_FS_ERR;
    }
    block_count =(uint8_t) super_block[SUPER_DISK_SIZE];
    if(block_count < 1){
        return TFS_INVALID_FS_ERR;
    }

    file_limit = (block_count-1)/2;    

    for(int i = 1; i < block_count; i++){
        status = readBlock(disk, i, block_buffer);
        if(status < 0){
            return status;
        }
        if(block_buffer[0] == 0 || block_buffer[0] > 4){
            return TFS_INVALID_FS_ERR;
        }
        if(block_buffer[1] != MAGIC_NUM){
            return TFS_INVALID_FS_ERR;
        }
        memset(block_buffer, 0, BLOCKSIZE*sizeof(char));
    }

    dynamic_file_list = calloc(256, sizeof(struct dynamic_t*));
    dynamic_table_maximum_size = 256;

    m_disk = disk; 
    return status;
}

int tfs_unmount(void){
    if(m_disk < 0){
        return 0;
    }
    
    for(int i = 0; i < dynamic_table_maximum_size; i++){
        if(dynamic_file_list[i]){
            if(dynamic_file_list[i]->file_data){
                free(dynamic_file_list[i]->file_data);
            }
            free(dynamic_file_list[i]);
        }
    }
    free(dynamic_file_list);
    
    int status = closeDisk(m_disk);
    if(status < 0){
        return status;
    }
    
    m_disk = -1;
    return status;
}

fileDescriptor tfs_openFile(char *name){
    uint8_t* block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    char* block_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    char* input_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    uint8_t* inode_map_buffer = (uint8_t*)calloc(BLOCKSIZE+1, sizeof(uint8_t));
    int status;
    int inode_block = -1;
    int found = 0;
    int filled = -1;
    status = tfs_fileNameValid(name);
    if(status < 0){
        return status;
    }
    status = readBlock(m_disk, 0,super_block);
    if(status < 0){
        return status;
    }
    status = readBitmap(super_block+SUPER_INODE_BLOCKS, inode_map_buffer, BLOCKSIZE);
    if(status < 0){
        return status;
    }
    strncpy(input_name, name, MAX_FILE_NAME_SIZE);
    int i, j;
    for(i = 0; i < strlen((char*)inode_map_buffer); i++){
        memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
        memset(block_name, 0, (MAX_FILE_NAME_SIZE + 1)*sizeof(uint8_t));
        int status = readBlock(m_disk, inode_map_buffer[i], block_buffer);
        if(status < 0){
            return status;
        }
        strncpy(block_name, (char*)(block_buffer+INODE_FILENAME), MAX_FILE_NAME_SIZE);
        if(block_buffer[0] == 0x02 && strncmp(input_name, block_name, 8) == 0){
            inode_block = inode_map_buffer[i];
            found = 1;
            break;
        }
    }
    if(found == 0){
        memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
        status = tfs_mkinode(block_buffer, BLOCKSIZE, name, time(NULL), FILE_READ_WRITE);
        if(status < 0){
            return status;
        }
        if(super_block[SUPER_FILE_COUNT] + 1 > file_limit){
            return TFS_TOO_MANY_FILE_ERR;
        }
        status = tfs_getFreeBlock(super_block);
        if(status < 0){
            return status;
        }
        time_t * write_time = (time_t *)(block_buffer+INODE_WRITE_TIME);
        write_time[0] = time(NULL);

        inode_block = status;
        status = writeBlock(m_disk, inode_block, block_buffer);
        if(status < 0){
            return status;
        }
        status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, inode_block);
        if(status < 0){
            return status;
        }
        status = modifyBitmap(super_block+SUPER_INODE_BLOCKS, inode_block);
        if(status < 0){
            return status;
        }
        super_block[SUPER_FILE_COUNT] += 1;
        status = writeBlock(m_disk, 0, super_block);
        if(status < 0){
            return status;
        }
    }
    for(j = 0; j < dynamic_table_maximum_size; j++){
        if(!dynamic_file_list[j]){
            filled = j;
            break;
        }
    }
    if(filled == -1){
        filled = dynamic_table_maximum_size;
        dynamic_table_maximum_size += 256;
        dynamic_file_list = realloc(dynamic_file_list, dynamic_table_maximum_size*sizeof(struct dynamic_t*));
        memset(dynamic_file_list+filled, 0, (256*sizeof(struct dynamic_t *)));
    }
    memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
    status = readBlock(m_disk, inode_block, block_buffer);
    if(status < 0){
        return status;
    }
    uint16_t * file_size_pointer = (uint16_t *)(block_buffer+INODE_FILESIZE);
    dynamic_file_list[filled] = malloc(sizeof(struct dynamic_t));
    dynamic_file_list[filled]->inode_num = inode_block;
    dynamic_file_list[filled]->offset = 0;
    dynamic_file_list[filled]->file_data = 0;
    dynamic_file_list[filled]->file_size = file_size_pointer[0];
    return filled;
}

int tfs_closeFile(fileDescriptor FD){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    struct dynamic_t * entry = dynamic_file_list[FD];
    dynamic_file_list[FD] = 0;
    if(entry->file_data){
        free(entry->file_data);
    }
    free(entry);
    return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t* inode_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* write_buffer = (uint8_t*)calloc(BLOCKSIZE-EXTENT_DATA, sizeof(uint8_t));
    uint8_t* extent_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* free_block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    if(inode_buffer[INODE_TYPE] == FILE_READ_ONLY){
        return TFS_ILLEGAL_WRITE_ERR;
    }
    status = readBlock(m_disk, 0, super_block);
    if(status < 0){
        return status;
    }
    int num_alloc_blocks = inode_buffer[INODE_NUM_BLOCKS];
    int num_needed_blocks = (int) size/(BLOCKSIZE-EXTENT_DATA);
    if(num_needed_blocks > BLOCKSIZE-INODE_DATA){
        return TFS_BLOCK_ADDRESS_LIMIT_ERR;
    }
    status = readBitmap(super_block + SUPER_FREE_BLOCKS, free_block_buffer, BLOCKSIZE);
    if(status < 0){
        return status;
    }
    if(num_needed_blocks > strlen((char*)free_block_buffer)){
        return TFS_INSUFFICIENT_FREE_BLOCK_ERR;
    }
    if(size%(BLOCKSIZE-EXTENT_DATA) != 0){
        num_needed_blocks+=1;
    }
    if(size > strnlen(buffer, size)+1){
        return TFS_INVALID_WRITE_SIZE;
    }
    if(size < 0){
        return TFS_INVALID_WRITE_SIZE;
    }

    int i;
    for(i = 0; i < num_needed_blocks; i++){
        if(i == num_alloc_blocks){
            status = tfs_getFreeBlock(super_block);
            if(status < 0){
                return status;
            }
            int free_block = status;
            status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, free_block);
            if(status < 0){
                return status;
            }
            status = writeBlock(m_disk, 0, super_block);
            if(status < 0){
                return status;
            }
            num_alloc_blocks++;
            inode_buffer[INODE_DATA+i] = free_block;
        }
        strncpy((char*)write_buffer, buffer+(i*(BLOCKSIZE-EXTENT_DATA)), BLOCKSIZE-EXTENT_DATA);
        status = tfs_mkextent(extent_block, BLOCKSIZE, write_buffer, BLOCKSIZE-EXTENT_DATA);
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, inode_buffer[INODE_DATA+i], extent_block);
        if(status < 0){
            return status;
        }
        memset(write_buffer, 0, (BLOCKSIZE-EXTENT_DATA)*sizeof(uint8_t));
        memset(extent_block, 0, BLOCKSIZE*sizeof(uint8_t));
    }
    for(i = 0; i < num_alloc_blocks - num_needed_blocks; i++){
        int target_block = inode_buffer[INODE_DATA+num_needed_blocks+i];
        status = readBlock(m_disk, target_block, write_buffer);
        if(status < 0){
            return status;
        }
        status = tfs_mkempty(write_buffer, BLOCKSIZE);
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, target_block, write_buffer);
        if(status < 0){
            return status;
        }
        status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, target_block);
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, 0, super_block);
        if(status < 0){
            return status;
        }
        inode_buffer[INODE_DATA+num_needed_blocks+i] = 0;
    }
    inode_buffer[INODE_NUM_BLOCKS] = num_needed_blocks;
    uint16_t * file_size = (uint16_t *)(inode_buffer + INODE_FILESIZE);
    file_size[0] = size;
    time_t * write_time = (time_t *)(inode_buffer+INODE_WRITE_TIME);
    write_time[0] = time(NULL);
    status = writeBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    dynamic_file_list[FD]->file_size = size;
    dynamic_file_list[FD]->file_data = 0;
    return 0;
}

int tfs_deleteFile(fileDescriptor FD){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t* inode_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* delete_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    int num_inode_block = dynamic_file_list[FD]->inode_num;
    int i;
    for(i = 0; i < dynamic_table_maximum_size; i++){
        if(dynamic_file_list[i] && i != FD){
            if(dynamic_file_list[i]->inode_num == num_inode_block){
                return TFS_FILE_IN_USE_ERR;
            }
        }
    }
    int status = readBlock(m_disk, num_inode_block, inode_buffer);
    if(status < 0){
        return status;
    }
    if(inode_buffer[INODE_TYPE] == FILE_READ_ONLY){
        return TFS_ILLEGAL_WRITE_ERR;
    }
    status = readBlock(m_disk, 0, super_block);
    if(status < 0){
        return status;
    }
    int num_extent_blocks = inode_buffer[INODE_NUM_BLOCKS];
    for(i = 0; i < num_extent_blocks; i++){
        int target_block = inode_buffer[INODE_DATA+i];
        status = readBlock(m_disk, target_block, delete_buffer);
        if(status < 0){
            return status;
        }
        status = tfs_mkempty(delete_buffer, BLOCKSIZE*sizeof(uint8_t));
        if(status < 0){
            return status;
        }
        status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, target_block); 
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, target_block, delete_buffer);
        if(status < 0){
            return status;
        }
        memset(delete_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
    }
    status = tfs_mkempty(inode_buffer, BLOCKSIZE*sizeof(uint8_t));
    if(status < 0){
        return status;
    }
    status = writeBlock(m_disk, num_inode_block, inode_buffer);
    if(status < 0){
        return status;
    }
    status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, num_inode_block);
    if(status < 0){
        return status;
    }
    status = modifyBitmap(super_block+SUPER_INODE_BLOCKS, num_inode_block);
    if(status < 0){
        return status;
    }
    super_block[SUPER_FILE_COUNT] -= 1;
    status = writeBlock(m_disk, 0, super_block);
    if(status < 0){
        return status;
    }

    status = tfs_closeFile(FD);
    return status;
}

int tfs_writeByte(fileDescriptor FD, unsigned int data){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    int eof_flag = 0;
    uint8_t* inode_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* extent_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* write_buffer = (uint8_t*)calloc(BLOCKSIZE-EXTENT_DATA, sizeof(uint8_t));    
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    uint16_t * file_size_location = (uint16_t *)(inode_buffer+INODE_FILESIZE);
    dynamic_file_list[FD]->file_size = file_size_location[0];
    if(inode_buffer[INODE_TYPE] == FILE_READ_ONLY){
        return TFS_ILLEGAL_WRITE_ERR;
    }
    status = readBlock(m_disk, 0, super_block);
    if(status < 0){
        return status;
    }
    if(dynamic_file_list[FD]->offset > dynamic_file_list[FD]->file_size){
        return TFS_OFFSET_PAST_FILE_EXTENT_ERR;
    }
    if(dynamic_file_list[FD]->offset > MAX_FILE_SIZE){
        return TFS_OFFSET_PAST_FILE_EXTENT_ERR;
    }
    if(dynamic_file_list[FD]->offset == dynamic_file_list[FD]->file_size){
        eof_flag = 1;
    }
	    if(dynamic_file_list[FD]->offset == dynamic_file_list[FD]->file_size &&  dynamic_file_list[FD]->file_size % (BLOCKSIZE-EXTENT_DATA) == 0){
        if(inode_buffer[INODE_NUM_BLOCKS] == BLOCKSIZE-INODE_DATA){
            return TFS_BLOCK_ADDRESS_LIMIT_ERR;
        }
        status = tfs_getFreeBlock(super_block);
        if(status < 0){
            return status;
        }
        int free_block = status;
        status = modifyBitmap(super_block+SUPER_FREE_BLOCKS, free_block);
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, 0, super_block);
        if(status < 0){

            return status;
        }
        status = tfs_mkextent(extent_buffer, BLOCKSIZE, write_buffer, BLOCKSIZE-EXTENT_DATA);
        if(status < 0){
            return status;
        }
        status = writeBlock(m_disk, free_block, extent_buffer);
        if(status < 0){
            return status;
        }
        inode_buffer[INODE_DATA+inode_buffer[INODE_NUM_BLOCKS]] = free_block;
        inode_buffer[INODE_NUM_BLOCKS] += 1;

    }

    int target_index = dynamic_file_list[FD]->offset/(BLOCKSIZE-EXTENT_DATA);
    int target_offset = dynamic_file_list[FD]->offset%(BLOCKSIZE-EXTENT_DATA);
    status = readBlock(m_disk, inode_buffer[INODE_DATA+target_index], extent_buffer);
    if(status < 0){
        return status;
    }

    extent_buffer[EXTENT_DATA + target_offset] = data;
    status = writeBlock(m_disk, inode_buffer[INODE_DATA+target_index], extent_buffer);
    if(status < 0){
        return status;
    }

    if(eof_flag == 1){
        dynamic_file_list[FD]->file_size += 1;
        uint16_t * file_size_location = (uint16_t *)(inode_buffer+INODE_FILESIZE);
        file_size_location[0] = dynamic_file_list[FD]->file_size;
    }

    time_t * write_time = (time_t *)(inode_buffer+INODE_WRITE_TIME);
    write_time[0] = time(NULL);
    status = writeBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    dynamic_file_list[FD]->file_data = 0;
    dynamic_file_list[FD]->offset += 1;
    return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t* inode_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* extent_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    uint16_t * file_size_location = (uint16_t *)(inode_buffer+INODE_FILESIZE);
    if(!dynamic_file_list[FD]->file_data){
        dynamic_file_list[FD]->file_size = file_size_location[0];
    }

    if(dynamic_file_list[FD]->offset > dynamic_file_list[FD]->file_size){
        return TFS_OFFSET_PAST_FILE_EXTENT_ERR;
    }else if(dynamic_file_list[FD]->offset == dynamic_file_list[FD]->file_size){
        return TFS_END_OF_FILE_ERR;
    }
    if(!dynamic_file_list[FD]->file_data){
        int num_blocks = dynamic_file_list[FD]->file_size/(BLOCKSIZE-EXTENT_DATA);
        if(dynamic_file_list[FD]->file_size % (BLOCKSIZE-EXTENT_DATA) != 0){
            num_blocks++;
        }
        uint8_t* file_data = (uint8_t*)calloc(num_blocks*(BLOCKSIZE-EXTENT_DATA), sizeof(uint8_t));
        int extent_block_count = inode_buffer[INODE_NUM_BLOCKS];
        for(int i = 0; i < extent_block_count; i++){
            status = readBlock(m_disk, inode_buffer[INODE_DATA+i], extent_buffer);
            if(status < 0){
                return status;
            }
            for(int k = 0; k < (BLOCKSIZE-EXTENT_DATA); k++){
                file_data[(i*(BLOCKSIZE-EXTENT_DATA)*sizeof(uint8_t))+k] = extent_buffer[EXTENT_DATA+k];
            }
            memset(extent_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
        }
        dynamic_file_list[FD]->file_data = file_data;
    }
    time_t * read_time = (time_t *)(inode_buffer+INODE_READ_TIME);
    read_time[0] = time(NULL);
    status = writeBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    buffer[0] = dynamic_file_list[FD]->file_data[dynamic_file_list[FD]->offset];
    dynamic_file_list[FD]->offset++;
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(offset < 0){
        return TFS_NEGATIVE_OFFSET_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    if(offset > dynamic_file_list[FD]->file_size){
        return TFS_OFFSET_PAST_FILE_EXTENT_ERR;
    }
    dynamic_file_list[FD]->offset = offset;
    return 0;
}

int tfs_makeRO(char * name){
    uint8_t* block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    char* block_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    char* input_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    uint8_t* inode_map_buffer = (uint8_t*)calloc(BLOCKSIZE+1, sizeof(uint8_t));
    int status;
    status = tfs_fileNameValid(name);
    if(status < 0){
        return status;
    }
    status = readBlock(m_disk, 0,super_block);
    if(status < 0){
        return status;
    }
    status = readBitmap(super_block+SUPER_INODE_BLOCKS, inode_map_buffer, BLOCKSIZE);
    if(status < 0){
        return status;
    }
    strncpy(input_name, name, MAX_FILE_NAME_SIZE);
    int i;
    for(i = 0; i < strlen((char*)inode_map_buffer); i++){
        memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
        memset(block_name, 0, (MAX_FILE_NAME_SIZE + 1)*sizeof(uint8_t));
        int status = readBlock(m_disk, inode_map_buffer[i], block_buffer);
        if(status < 0){
            return status;
        }
        strncpy(block_name, (char*)block_buffer+INODE_FILENAME, MAX_FILE_NAME_SIZE);
        if(block_buffer[0] == 0x02 && strncmp(input_name, block_name, 8) == 0){
            int inode_block = inode_map_buffer[i];
            block_buffer[INODE_TYPE] = FILE_READ_ONLY;
            status = writeBlock(m_disk, inode_block, block_buffer);
            return 0;
        }
    }
    return TFS_NON_EXISTANT_FILE_ERR;
}

int tfs_makeRW(char * name){
    uint8_t* block_buffer = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    uint8_t* super_block = (uint8_t*)calloc(BLOCKSIZE, sizeof(uint8_t));
    char* block_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    char* input_name = (char*)calloc(MAX_FILE_NAME_SIZE + 1, sizeof(char));
    uint8_t* inode_map_buffer = (uint8_t*)calloc(BLOCKSIZE+1, sizeof(uint8_t));
    int status;
    status = tfs_fileNameValid(name);
    if(status < 0){
        return status;
    }
    status = readBlock(m_disk, 0,super_block);
    if(status < 0){
        return status;
    }
    status = readBitmap(super_block+SUPER_INODE_BLOCKS, inode_map_buffer, BLOCKSIZE);
    if(status < 0){
        return status;
    }
    strncpy(input_name, name, MAX_FILE_NAME_SIZE);
    int i;
    for(i = 0; i < strlen((char*)inode_map_buffer); i++){
        memset(block_buffer, 0, BLOCKSIZE*sizeof(uint8_t));
        memset(block_name, 0, (MAX_FILE_NAME_SIZE + 1)*sizeof(uint8_t));
        int status = readBlock(m_disk, inode_map_buffer[i], block_buffer);
        if(status < 0){
            return status;
        }
        strncpy(block_name, (char*)(block_buffer+INODE_FILENAME), MAX_FILE_NAME_SIZE);
        if(block_buffer[0] == 0x02 && strncmp(input_name, block_name, 8) == 0){
            int inode_block = inode_map_buffer[i];
            block_buffer[INODE_TYPE] = FILE_READ_WRITE;
            status = writeBlock(m_disk, inode_block, block_buffer);
            return 0;
        }
    }
    return TFS_NON_EXISTANT_FILE_ERR;
}

time_t tfs_readFileInfo(fileDescriptor FD){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t * inode_buffer = (uint8_t *)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    time_t * read_time_buffer = (time_t *)(inode_buffer+INODE_CREATION_TIME);
    return read_time_buffer[0];
}


time_t tfs_lastReadTime(fileDescriptor FD){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t * inode_buffer = (uint8_t *)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    time_t * read_time_buffer = (time_t *)(inode_buffer+INODE_READ_TIME);
    return read_time_buffer[0];
}

time_t tfs_lastWriteTime(fileDescriptor FD){
    if(FD < 0 && FD > dynamic_table_maximum_size){
        return TFS_INVALID_FD_ERR;
    }
    if(!dynamic_file_list[FD]){
        return TFS_INVALID_FD_ERR;
    }
    uint8_t * inode_buffer = (uint8_t *)calloc(BLOCKSIZE, sizeof(uint8_t));
    int status = readBlock(m_disk, dynamic_file_list[FD]->inode_num, inode_buffer);
    if(status < 0){
        return status;
    }
    time_t * read_time_buffer = (time_t *)(inode_buffer+INODE_WRITE_TIME);
    return read_time_buffer[0];
}

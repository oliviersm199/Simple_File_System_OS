#include "disk_emu.h"
#include "sfs_api.h"

#include <stdio.h>
#include <stdlib.h>



superblock_t sb;
inode_table i_table;
file_descriptor fdt[NUM_INODES];
bitmap bm;

void init_superblock(){
	sb.magic = 0xACBD0005;
	sb.block_size = BLOCK_SZ;
	sb.fs_size = NUM_BLOCKS * BLOCK_SZ;
	sb.inode_table_len = NUM_INODE_BLOCKS;
	sb.root_dir_inode = 0;
}
//bitmap functions
void init_bitmap(){
	int i;
	for(i=0;i<NUM_BLOCKS;i++){
	    bm.index[i] = BLOCK_AVALIABLE;
	}
	bm.first_free_block = 0;
}
	
int occupy_block(){
    int return_value =  bm.first_free_block;    
    //updating the last_free_block parameter
    for(int i =bm.first_free_block+1;i<NUM_BLOCKS;i++){
	if(bm.index[i]==BLOCK_AVALIABLE){
            bm.first_free_block=i;
	    break;
	}
    }
    if(return_value == bm.first_free_block){
	printf("No room in memory\n");
	return -1; 
    }
    bm.index[return_value] = BLOCK_OCCUPIED;
    write_blocks(BITMAP_START,NUM_BITMAP_BLOCKS,&bm);
    return return_value; 
}

int release_block(int release){
	//block is free to overwrite.
	if(bm.index[release]==BLOCK_OCCUPIED){
		bm.index[release] = BLOCK_AVALIABLE;
		if(bm.first_free_block>release){
			bm.first_free_block=release;
		}
		write_blocks(BITMAP_START,NUM_BITMAP_BLOCKS,&bm);
		return 0;
	}
	printf("Releasing block that has not been allocated\n");
	return -1;
}

//root directory
void init_rootdir(){
    
}

//inode functionalities
void init_inode_table(){
    for(int i = 0 ; i<NUM_INODES;i++){
	i_table.index[i].inuse = INODE_FREE;
    }
    i_table.first_free_inode = 0;	
}

int occupy_inode(){
    int return_value = i_table.first_free_inode;
    for(int i =return_value+1;i<NUM_INODES;i++){
	if(i_table.index[i].inuse == INODE_FREE){
		i_table.first_free_inode = i;
		break;
	}
    }
    if(return_value == i_table.first_free_inode){
	printf("%d", i_table.first_free_inode);
	printf("No free inode\n");
        return -1;
    }
    i_table.index[return_value].inuse =INODE_IN_USE;
    write_blocks(1,sb.inode_table_len,&i_table);
    return return_value;
}

int release_inode(int release){
    if(i_table.index[release].inuse==INODE_IN_USE){
      	i_table.index[release].inuse = INODE_FREE;
        if(i_table.first_free_inode> release) i_table.first_free_inode = release;
	return 0;    
    }
    printf("Trying to release unused iNode\n");
    return -1;
}


inode_t* create_empty_inode(){
	int inode_index = occupy_inode();
	if(inode_index<0){
		return NULL;
	}
	inode_t * newInode = &i_table.index[inode_index];
	for(int i =0;i<NUM_DATAPTRS;i++){
		newInode -> data_ptrs[i] = NULL; 
	}
	newInode -> size = 0;
	write_blocks(1,sb.inode_table_len,&i_table);
	
	return newInode;
}




//create the file system
void mksfs(int fresh){
	if(fresh){
		printf("Making new file system\n");
		//initializing the superblock data
		init_superblock();
		init_fresh_disk(OLIS_DISK,BLOCK_SZ,NUM_BLOCKS);
		init_inode_table();
		init_bitmap();
		//writing the superblock and setting to occupied in the bitmap
		write_blocks(0,1,&sb);
		bm.index[0] = BLOCK_OCCUPIED;
		
		//writing the inode table and setting to occupied in the bitmap
		write_blocks(1,sb.inode_table_len,&i_table);
		for(int i =1;i<sb.inode_table_len+1;i++){
		  bm.index[i]=BLOCK_OCCUPIED;
		}
		
		//writing bitmap and setting last blocks to the bitmap blocks
		for(int i =BITMAP_START;i<BITMAP_START+NUM_BITMAP_BLOCKS;i++){
		  bm.index[i] = BLOCK_OCCUPIED;
		}
		write_blocks(BITMAP_START,NUM_BITMAP_BLOCKS,&bm);

		//initializing the rootdir
	}
	else{
		printf("reopening file system\n");
		read_blocks(0,1,&sb);
		printf("Block Size is: %lu\n",sb.block_size);
		//open inode table
		read_blocks(1,sb.inode_table_len,&i_table);
	}

	return;
}

int sfs_getnextfilename(char *fname) {
	//Implement sfs_getnextfilename here
	return 0;
}
int sfs_getfilesize(const char* path) {
	//Implement sfs_getfilesize here
	return 0;
}
int sfs_fopen(char *name) {
		//Implement sfs_fopen here

    /*
     * For now, we only return 1
     */
    uint64_t test_file = 1;
    fdt[test_file].inode = 1;
    fdt[test_file].rwptr = 0;
		return test_file;
}

int sfs_fclose(int fileID){

	//Implement sfs_fclose here
    fdt[0].inode = 0;
    fdt[0].rwptr = 0;
		return 0;
}

int sfs_fwrite(int fileID, const char *buf, int length){
	//file_descriptor* f = &fdt[fileID];
	//inode_t* n = &table[fileID];
	/*
	 *We know block 1 is free because this is a canned example.
	 *You should call a helper function to find a free block.
	 */
	//int block = 20;
	//n -> data_ptrs[0] = block;
	//n -> size += length;
	//f -> rwptr += length;
	//write_blocks(block,1,(void *) buf);
	return 0;
}

int sfs_fread(int fileID, char *buf, int length){
			//Implement sfs_fread here
	    //file_descriptor* f = &fdt[fileID];
	    //inode_t* n = i_table.[f->inode];

	    //int block = n->data_ptrs[0];
	    //read_blocks(block, 1, (void*) buf);
	    return 0;
}


int sfs_fseek(int fileID, int loc){
	//MINIMAL IMPLEMENTATION, ADD SOME TYPE OF ERROR CHECKING
	fdt[fileID].rwptr = loc;
	return 0;
}

int sfs_remove(char *file){
	//implement sfs_remove here
	return 0;
}

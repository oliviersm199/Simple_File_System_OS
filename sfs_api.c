#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
superblock_t sb;

inode_table i_table;

bitmap bm;

root_directory root_dir;

fdt_table f_table;

int dir_size = sizeof(root_dir.list)/sizeof(root_dir.list[0]);
int fdt_size = sizeof(f_table.fdt)/sizeof(f_table.fdt[0]);
int i_table_size = sizeof(i_table.index)/sizeof(i_table.index[0]);
int bm_size = sizeof(bm.index)/sizeof(bm.index[0]);

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
    for(i=0;i<bm_size;i++){
        bm.index[i] = BLOCK_AVALIABLE;
    }
    bm.first_free_block = 0;
}
	
int occupy_block(){
    int return_value =  bm.first_free_block;    
    //updating the last_free_block parameter
    for(int i =bm.first_free_block+1;i<bm_size;i++){
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
    if(release>=0 && release<bm_size){
        if(bm.index[release]==BLOCK_OCCUPIED){
            bm.index[release] = BLOCK_AVALIABLE;
	    if(bm.first_free_block>release){
		bm.first_free_block=release;
	    }
	    write_blocks(BITMAP_START,NUM_BITMAP_BLOCKS,&bm);
	    return 0;
	}
	else{
	    printf("Releasing block that has not been allocated\n");
	    return -1;
	}
    }
    printf("Releasing block that is out of range\n");
    return -2;
}

int get_block_assignments(inode * i_ptr,int blocks_needed){
    //finding out the start of new blocks
    int data_ptrs = i_ptr -> data_pointers_used;
    int max_index = data_ptrs + blocks_needed;
    int indirect_data_ptrs_used = (data_ptrs-SINGLE_PTR_NUM<0) 0: data_ptrs - SINGLE_PTR_NUM;

    if(max_index > MAX_DATA_PTRS){
	return -1;	
    }
    int blocks_assigned[blocks_needed]; 
    for(int i =0; i< blocks_needed;i++){
	blocks_assigned[i] = occupy_block();
	if(blocks_assigned[i]<0){
	    for(int j=0;j<=i;j++){
		release_block(blocks_assigned[i]);
	    }
	    return -2;
	}
    }
    int j = 0; 
    for(int i = data_pointers_used;i<blocks_needed;i++){
	if(i<MAX_DATA_PTRS-1){
	    i_ptr.data_ptrs[i]=blocks_assigned[j];
	    j++;
        }
        else{
	    if(i_ptr.indirect_ptr<0){
		i_ptr.indirect_ptr = occupy_block();
		if(i_ptr.indirect_ptr < 0){
		    for(int q = 0;q<blocks_needed;q++){
			release_blocks(blocks_assigned[q]);
		    }
		    return -3;
		}	
	    }
	    int temp_inode_array[MAX_DATA_PTRS-SINGLE_PTR_NUM]
	    read_block(i_ptr.indirect_ptr,1,temp_inode_array);
	    while(j<blocks_needed){
		temp_inode_array[indirect_data_ptrs_used] = blocks_assigned[j];
		j++;
	    }
	    break;
        }
    }
    return 0;      
}


//inode functionalities
void init_inode_table(){
    for(int i = 0 ; i<i_table_size;i++){
	i_table.index[i].inuse = INODE_FREE;
    	i_table.data_ptrs_used = 0;
    }
    i_table.first_free_inode = 0;	
}

int occupy_inode(){
    int return_value = i_table.first_free_inode;
    for(int i =return_value+1;i<i_table_size;i++){
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
    if(release>=0 && release < i_table_size){
    if(i_table.index[release].inuse==INODE_IN_USE){
      	i_table.index[release].inuse = INODE_FREE;
        if(i_table.first_free_inode> release) i_table.first_free_inode = release;
	    return 0;    
    	}
        else{
            printf("Trying to release unused iNode\n");
            return -1;
        }
    }
    printf("Invalid index for inode");
    return -2;
}


int create_empty_inode(){
    int inode_index = occupy_inode();
    if(inode_index<0){
	return -1;
    }
    inode_t * newInode = &i_table.index[inode_index];
    newInode -> size = 0;
    write_blocks(1,sb.inode_table_len,&i_table);
    return inode_index;
}

//everytime we change the dataptrs of a file, we need to save this to the disk.
void save_inode_table(){
    write_blocks(1,sb.inode_table_len,&i_table);
}

//root directory functions
void init_root(){
    //creating empty inode which will be in position 0 since this is called before anything else. 
    sb.root_dir_inode = create_empty_inode();
}

void init_directory(){
    for(int i =0;i<dir_size;i++){
        root_dir.list[i].inode_ptr=-1;
    }   
}



/*returns the index of a file if it exists otherwise returns -1*/
int file_exists(char * filename){
    
    for(int i = 0; i< dir_size;i++){
    	char *current_filename = root_dir.list[i].filename;	
	if(strcmp(current_filename,filename)==0){
            return i;
	} 
    }
    return -1; 
}


int new_file_dir(char *filename,int inode_num){
    int root_num = root_dir.first_free;
    root_dir.list[root_num].inode_ptr = inode_num;

    for(int i = root_dir.first_free;i<dir_size;i++){
	if(root_dir.list[i].inode_ptr<0){
	    root_dir.first_free = i; 
            break;
	}
    }
    if(root_num == root_dir.first_free){
	printf("Root Directory is full\n");
	root_dir.list[root_num].inode_ptr = inode_num;
        return -1;
    }
    strncpy(root_dir.list[root_num].filename,filename,strlen(filename));
    return root_num;
}

int delete_file_dir(int dir_pos){
    if(dir_pos<0||dir_pos>dir_size-1) return -1;
    root_dir.list[dir_pos].inode_ptr =-1;
    strcpy(root_dir.list[dir_pos].filename,"");
    return 0; 
}


//fdt directory functions
void init_fdt(){
   f_table.next_free = 0;
   for(int i = 0; i<fdt_size;i++){
       
       f_table.fdt[i].inode = -1;
       f_table.fdt[i].wptr = 0;
       f_table.fdt[i].rptr = 0;
   } 
}

int new_fd(int inodeNum){
    int new_fdt = f_table.next_free;
    f_table.fdt[new_fdt].inode = inodeNum;

    for(int i = f_table.next_free;i<fdt_size;i++){
	if(f_table.fdt[i].inode < 0){
            f_table.next_free = i;
	    break;
        }
    }

    if(f_table.next_free == new_fdt){
        printf("The file descriptor table is full, can no longer add.\n");
        f_table.fdt[new_fdt].inode= -1;
        return -1;
    }
    f_table.fdt[new_fdt].inode = inodeNum; 
    f_table.fdt[new_fdt].rptr = 0;
    f_table.fdt[new_fdt].wptr = 0;
    return new_fdt; 
}

int remove_fd(int file_ptr){
    if(file_ptr<0 || file_ptr>fdt_size - 1){
	printf("Invalid index for deleting a file descriptor\n");
        return -1;
    }
    f_table.fdt[file_ptr].inode = -1;
    f_table.fdt[file_ptr].wptr = 0;
    f_table.fdt[file_ptr].rptr=0;
 
    if(file_ptr < f_table.next_free){
        f_table.next_free = file_ptr;
    }
    return 0; 
}

int verify_in_fd(int inode){
for(int i =0;i<fdt_size;i++){
    if(f_table.fdt[i].inode==inode){
	return i;
    }
  }
  return -1;
}



//various helper functions
int validate_filename(char * name){
    int str_len = strlen(name);
    if(!str_len){
        return 0;
    }

    //verifying dot position
    char * dot = strchr(name, '.');

    //scenario if dot exists
    if(dot == NULL){
        if(str_len<=20) return 1;
        return 0;
    }
    //getting index by calculating offset of dot ptr and char ptr.
    int index = dot - name;
    int count = 0;

    while(count+index<str_len){
        count++;
    }

    //checking if characters including dot are proper length
    if(count > 4){
        return 0;
    }

    //checking if strlen is proper.
    if(strlen(name)-count>16){
        return 0;
    }
    return 1;
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
	
	init_root();
	init_fdt();
        init_directory();
	}
	else{
	    printf("reopening file system\n");
	    read_blocks(0,1,&sb);
	    //open inode table
            read_blocks(1,sb.inode_table_len,&i_table);
	    read_blocks(BITMAP_START,NUM_BITMAP_BLOCKS,&bm);
	    init_fdt();
	}
    return;
}

int sfs_getnextfilename(char *fname) {
    int count = root_dir.next;
    for(int i=0;i<dir_size;i++){
	if( root_dir.list[i].inode_ptr>=0 && count==0 ){
            strcpy(fname,root_dir.list[i].filename);
	    root_dir.next++;
            return 1;
        }
        else if(root_dir.list[i].inode_ptr>=0){
	    count--;
	}
    } 
    root_dir.next = 0; 
    return 0;
}

int sfs_getfilesize(const char* path) {
    if(!validate_filename((char *)path)){
        return -1;
    }
    //check if file already exists
    int file_number = file_exists((char *)path);
    if(file_number<0){
	return -2;
    } 
    
    int inode_number = f_table.fdt[file_number].inode;
    return i_table.index[inode_number].size;
}


int sfs_fopen(char *name) {  
    
    if(!validate_filename(name)){
    	return -1;
    }
    
    int file_number = file_exists(name);   
    if(file_number<0){
        
        int inode_num = create_empty_inode();
	
        if(inode_num<0) return -1;
        
        int fdt_num = new_fd(inode_num);
 
	if(fdt_num<0) return -2;
	f_table.fdt[fdt_num].inode = inode_num;
	f_table.fdt[fdt_num].rptr = 0;
	f_table.fdt[fdt_num].wptr = 0;
        int file_dir_num = new_file_dir(name,inode_num);
        if(file_dir_num<0) return -3;

        return fdt_num;		
    }
    else{
	int inode_ptr = root_dir.list[file_number].inode_ptr;
        int infd = verify_in_fd(inode_ptr);
        if(infd>=0){
            return -4;
	}

        int file_desc = new_fd(inode_ptr);
	if(file_desc<0){
	    return -5;
	}

	return file_desc; 	
    }
}


int sfs_fclose(int fileID){
    //Implement sfs_fclose here
    return remove_fd(fileID);
}

int sfs_fwrite(int fileID, const char *buf, int length){
	if(fileID<0||fileID >=fdt_size){
	    return -1;
        }
	
	int inode = f_table.fdt[fileID].inode;
	if(inode<0){
	  return -2;
        }
	inode_t * n = &i_table.index[inode];
        file_descriptor * f = &f_table.fdt[fileID];
	int filesize = n -> size;
	int blocks_used = filesize/BLOCK_SIZE+1;
        int wptr_start_block = f.wptr/BLOCK_SIZE;
	int blocks_writing = length/BLOCK_SIZE+1;
	int blocks_needed = (blocks_writing-blocks_used)+wptr_start_block;
	
        if(get_block_assignments(inode* n,blocks_needed)<0){
	    return -3; 
        }
	int buf_written = 0;
	char * temp_buffer[BLOCK_SIZE];
	for(int i =wptr_start_block;i<number_blocks_to_write+wptr_start_block;i++){
	    int write_start = (f.wptr%BLOCK_SIZE);
	    int block = n -> data_ptrs[i];
	    read_blocks(block,1,(void*) temp_buffer);
	    int write_amount = BLOCK_SIZE - write_start;
	    strncpy(temp_buffer+write_start,buf+buf_written,write_amount); 
	    buf_written += write_amount;
            n -> size += (write_amount)
	    n -> wptr += (write_amount);
	    write_blocks(block,1,(void *) temp_buffer);
	}
	return buf_written;
}

int sfs_fread(int fileID, char *buf, int length){
    if(fileID<0 || fileID >=fdt_size){
        return -1;
    }
    int inode = f_table.fdt[fileID].inode;
    if(inode<0){
	return -2;
    }
    inode_t * n = &i_table.index[inode];
    int block = n->data_ptrs[0];
    read_blocks(block, 1, (void*) buf);
    return 0;
}


int sfs_fseek(int fileID, int loc){
	//MINIMAL IMPLEMENTATION, ADD SOME TYPE OF ERROR CHECKING
	f_table.fdt[fileID].wptr = loc;
        f_table.fdt[fileID].rptr = loc;
        return 0;
}

int sfs_remove(char *file){
	//implement sfs_remove here
	return 0;
}

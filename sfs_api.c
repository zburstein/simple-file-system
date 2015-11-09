#include "sfs_api.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#define BLOCK_SIZE 512
#define MAX_BLOCKS 100

#define DISK_FILE "sfs_disk.disk"
#define MAX_INODES 10 //not sure what this should be
#define MAX_FILES 10 //not sure wha thtis should be

void zeroDisk();
void init_sb();
void add_root_dir_inode();
void init_free_blocks();
int findInDir(const char *path);
void findOpenInode(int number);
void findOenDir(int number);
void findOpenFd(int number);
void findCurrentBlock(int number);

super_block_t sb; //the super block
dir_entry_t root_dir[MAX_INODES]; //array of directory entris will be the directory structure
inode_t inode_table[MAX_INODES]; //the inode table will be array of inodes
fd_table_entry_t fd_table[MAX_FILES]; //the file descriptor table
unsigned short free_blocks[MAX_BLOCKS]; //free blocks jsut an int array with each index mapped to corresponding block
int currentBlock; //to know what block to write the data to
int availableInode; //will represent the lowest value inode space available
int availableDirectory; //should i use this
int availableFd;

//rot directory inode: mode? size? ptrs?
//does root directory have an entry for itself within itself?
//instead of using current block should i just use the freeblocks array? 
//what if we run out of inode or directory sapce in block?
//in conjunction with previous question what if we alot and then it happens to be bigger than that
//are those that i declare to keep track of entries in fd table bad practice? 
//what should we set the pointer for data blocks to when we are not allocating it any space
//he created close as having int return but not that in the assignment. So which is it? What should failure and success codes be
//do we have to malloc a space for the buffer. I dont understand when we are supposed to be doing this
//also whats the deal with caching
//need to figure out data ptrs and what to do when there is more tan one block of memory being used. might just be aloop. but dont really know how data ptrs work so cant implement it
//Check the usages of the available stuff to ensure that I have the properc checks before using them
//where should write and read pointers start? 
//What does he mean flush it to disk when writing
//do we ahve to assign EOF
//everythign i have is set up for each file only having a single block. ill have to make changes once i figure out what is going on with that. Especially with write. alot going on there. Will ahve to do a bunch of checks probably 
//are we seting an upper limit on our file size. if not then how to properly deal with the data ptrs.
//how many blocks is the directory
//what am i returning for seek
//what if try and remove a file that is open. how to get fdtable entry
//do we need to write 0s into the data blocks?
//do i want currentBlock

//remove, write, and read are all set up for single blocks. will need to change these once I understand the pointers

void mksfs(int fresh) {
	//Implement mksfs here	
	if(fresh == 1){
		init_fresh_disk(DISK_FILE, BLOCK_SIZE, MAX_BLOCKS);//initializes the disk of given name and of the size
		zeroDisk(); //zero all contents 
		
		//create superblock and write it to disk
		init_sd(); //initialize the super block
		write_blocks(0, 1, &sb); //write to block 0 the contents of sb		

		//initialize inode table
		add_root_dir_inode(); //create root inode
		write_blocks(1, 1, &inode_table); //write the inode table
		availableInode = 1;

		//create and write root dir
		write_blocks(2, 1, &root_dir);
		availableDir = 0; //||1 depends on if it has its own entry

		//init the free blocks array and write it
		init_free_blocks();
		write_blcoks(MAX_BLOCKS - 1, 1, &free_blocks);//now write it to
		currentBlock = 3;
	}
	else {
		////?????
	}
	return;
}

int sfs_getnextfilename(char *fname) {
	//Implement sfs_getnextfilename here	

	return 0;
}


int sfs_getfilesize(const char* path) {
	//Implement sfs_getfilesize here	
	
/*
	int i, isPresent = 0;	
	//loop through until find it
	for(i = 0; i < MAX_INODES; i++){
		if(strcmp(root_dir[i].file_name, path) == 0){
			isPresent = 1;
			break;
		}
		else if(root_dir == NULL){
			break;
		}
	}

	if(isPresent)
		return root_dir[i].inode.size;
	else{
		printf("File does not exist\n");
		return -1;
	}*/
	
	int index = findInDir(path);
	if(index >= 0)
		return root_dir[i].inode.size; //this might not work becaseu size might not be a pointer but rather index in inode table/inode_table[root_dir[i].inode].size
	else{
		printf("File not found\n");
		return -1;
	}	
	
}

int sfs_fopen(char *name) {
	//Implement sfs_fopen here	
	int inodeIndex, i, fd;
	
	int index = findInDir(name);
	//if it does not exist then we need to create it by creating an inode and placig it in hte inode table as well as placing it in the directory
	if(index == -1){ //does not exist so need to create the file
		/*
		//find first null inode value
		for(inodeIndex = 0; inodeIndex < MAX_NODES; inodeIndex++){
			if(inode_table[inodeIndex] == NULL){
				//place new inode
				inode_table[inodeIndex].mode = 0x755;///
				inode_table[inodeIndex].link_cnt = 1;
				inode_table[inodeIndex].uid = 0;
				inode_table[inodeIndex].gid = 0;
				inode_table[inodeIndex].size = 0;
				inode_table[inodeIndex].data_ptrs[0] = currentBlock;
				currentBlock++;
			}
		}
		*/
		
		//first check if space in inode table
		if(availableInode == MAX_INODES || currentBlock == MAX_BLOCKS || availableDir == MAX_INODES){
			printf("There is no space available for another file\n");
			return -1;
		}

		//else place new inode
		inode_table[availableInode].mode = 0x755;
		inode_table[availableInode].link_cnt = 1;
		inode_table[availableInode].uid = 0;
		inode_table[availableInode].gid = 0;
		inode_table[availableInode].size = 0;
		inode_table[availableInode].data_ptrs[0] = -1; //currentBlock; //place in newest available block

		//and do the free_blocks
		//free_blocks[currentBlock]  = 1;
		//currentBlock++; //increment

		//create new directory entry
		//???? check fo valid 
		if(strlen(path) > 20){ //|| check also for proper extension length
			printf("File name is invalid. Must not be longer than 20 char total and extension cannot be longer than 3 char\n");
			return -1;
		}
		strcpy(root_dir[availableDir].file_name, path); //copy the file name
		root_dir[availableDir].inode = availableInode;	//need to be pointer or index?
		
		//change the values of availableInode and availableDIr accordingly for next entry
		findOpenInode(availableInode);
		findOpenDir(availableDir); 
	}	

	//file already exists or has been created at this point
	//need to give file an entry in the file descriptor table
	//search directory
	for(i = 0; i < MAX_INODES; i++){
		//when find it set all the properties in fdtable accordingly
		if(strcmp(root_dir[i].file_name, path) == 0){
			fd_table[availableFd].inode_number = root_dir[i].inode; //in fd table set the inode number
			fd_table[availableFd].rw_pointer= inode_table[root_dir[i].inode].size; //set read write pointer to end of file
		}
	}

	fd = availableFd; //set the file descriptor index
	findOpenFd(availableFd);//update availe fd index
	return fd;//return file descriptor for the file opened
}

int sfs_fclose(int fileID){
	//Implement sfs_fclose here	
	//remove the file from the fd table
	fd_table[fileID].inode_number = 0;
	fd_table[fileID].rw_pointer = 0;

	//if the file descriptor being moved is the lowest value now open in fd_table
	if(fileID < availableFd)
		availableFd = fileID;

	return 0;
}

int sfs_fread(int fileID, char *buf, int length){
	//find reference within the file descriptor table and get the inode
	int inodeNumber, block_number;
	char blockContent[BLOCK_SIZE];
	inodeNumber = fd_table[fileId].inode_number;	
	
	//if bad handle
	if(inodeNumber == 0){
		printf("File descriptor does not exist\n");
		return 0; //return error
	}
	
	//if file is empty
	if(inode_table[inodeNumber].size == 0){
		strcpy(buf, "");
		return 0;
	}

	//if too long just cut it down to proper size
	if(fd_table[fileID].rw_pointer + length > inode_table[inodeNUmber].size)
		length = inode_table[inodeNumber].size - fd_table[fileID].rw_pointer;
	
	//read the block into a buffer to later copy specific contents		
	read_blocks(inode_table[inodeNumber].data_ptrs[0], 1, &blockContent[0]);
	
	//then copy the desired amount into buf starting from the read pointer
	memcpy(buf, &blockContent[fd_table[fileID].rw_pointer], length); 
	fd_table[fileID].rw_pointer += length; //update the read pointer

	return length; //return the number of characters copied
}

int sfs_fwrite(int fileID, const char *buf, int length){
	//Implement sfs_fwrite here	
	int inodeNumber;
	char blockContent[BLOCK_SIZE];	

	inodeNumber = fd_table[fileID].inode_number;
	//if bad handle
	if(inodeNumber == 0){
		printf("File descriptor does not exist\n");
		return 0;
	}

	//if the file is empty, need to allot it a block
	if(inode_table[inodeNumber].size == 0){
		//if full
		if(currentBlocks >= MAX_BLOCKS){
			printf("No space available\n");
			return 0;
		}

		//give it a block and adjust block values accordingly
		inode_table[inodeNumber].data_ptrs[0] = currentBlock;
		free_blocks[currentBlock] = 1;
		findCurrentBlock(currentBlock);
	}

	//if will fit in the block. THIS IS FOR IF ONLY ONE BLOCK
	if((inode_table[inodeNumber].size + length) / BLOCK_SIZE <= 1){
		read_blocks(inode_table[inodeNumber].data_ptrs[0], 1, &blockContent[0]); //read the block
		memcpy(&blockContent[fd_table[fileID].rw_pointer], buf, length); //copy into blcok content at write pointer hte contents of buf
		write_blocks(inode_table[inodeNumber].data_ptrs[0], 1, &blockContent[0]);//write into the block
		inode_table[inodeNumber].size += length; //update the size
		fd_table[fileID].rw_pointer += length; //update the r/w pointer
	}

	//need to know how the data ptrs work to do the rest   
	
		

	return 0;
}

int sfs_fseek(int fileID, int loc){
	//Implement sfs_fseek here	
	int inodeNumber;

	inodeNumber = fd_table[fileID].inode_number; //get inode number 

	//if provided locaiton to large
	if(loc > inode_table[inodeNumber].size){
		fd_table[fileID].rw_pointer = inode_table[inodeNumber].size;
	}
	//if too small
	else if(loc < 0){
		fd_table[fileID].rw_pointer = 0;
	}
	else{
		fd_table[fileID].rw_pointer = loc;
	}

	return 0;
}

int sfs_remove(char *file) {
	//Implement sfs_remove here	
	int dirIndex, inodeNumber;

	//find the file in directory
	dirIndex = findInDir();
	if(dirIndex == -1){
		printf("File does not exist\n");
		return EXIT_FAILURE;
	}

	inodeNumber = root_dir[dirIndex].inode;//get inode number

	//remove the data blocks
	free_blocks[inode_table[inodeNumber].data_ptrs[0]] = 0;
	
	//reset current block if it is not the earliest available
	if(currentBlock > inode_table[inodeNumber].data_ptrs[0]){
		currentBlock = inode_table[inodeNumber].data_ptrs[0];
	}

	//remove the inode 
	inode_table[inodeNumber].mode = 0;
	inode_table[inodeNumber].link_cnt = 0;
	inode_table[inodeNumber].uid = 0;
	inode_table[inodeNumber].gid = 0;
	inode_table[inodeNumber].size = 0;
	inode_table[inodeNumber].data_ptrs[0] = 0;

	//reset availableInode
	if(availableInode > inodeNumber){
		availableInode = inodeNumber;
	}

	//remove the directory entry
	root_dir[dirIndex].file_name = 0;
	root_dir[dirIndex].inode = 0;

	//reset direntry if needed
	if(availableDirectory > dirIndex){
		availableDirectory = dirIndex;
	}
	
	return 0;
}

void init_sb(){
	sb.magic = 1234;
	sb.block_size = BLOCK_SIZE;
	sb.fs_size = MAX_BLOCKS * BLOCKS_SIZE;
	sb.inode_table_length = MAX_INODES;
	sb.root_dir_inode = 0; //why is this 0
}

void zeroDisk(){
	//some of this seems off like the directory. Also why no zeroing of the data portion
	bzero(&sb, sizeof(super_block_t)); //zero the super block
	bzero(&inode_table[0], sizeof(inode_t) * MAX_INODES); //inode table
	bzero(&root_dir, sizeof(dir_entry_t)); //root directory
	bzero(&free_blocks[0], sizeof(unsigned short) * MAX_BLOCKS); //free blocks array
	bzero(&fd_table[0], sizeof(fd_entry_t) * MAX_BLOCKS); //fd table
}

void add_root_dir_inode(){
	inode_table[0].mode = 0x755; //why this?
	inode_table[0].link_cnt = 1;
	inode_table[0].uid = 0; //we dont have users
	inode_table[0].gid = 0; //dont have groups
	inode_table[0].size = 0; //what is the size
	inode_table[0].data_ptrs[0] = 2; //store in third block
}

void init_free_blocks(){

	//mark the blocks tahta are used
	free_blocks[0] = 1; //the superblok
	free_blocks[1] = 1; //the inode table
	free_blocks[2] = 1; //the root directory
	free_blocks[MAX_BLOCKS - 1] = 1; //the free blocks array
}

int findInDir(const char *path){

	int i, isPresent = 0;
	//loop through until find it
	for(i = 0; i < MAX_INODES; i++){
		if(strcmp(root_dir[i].file_name, path) == 0){
			isPresent = 1;
			break;
		}
		else if(root_dir[i].file_name == 0)
			break;
	}

	if(isPresent)
		return i;
	else
		return -1;
}
	
void findOpenInode(int number){
	int i = number;
	for(i = number; i < MAX_INODES; i++){//can start at the current location
		//if it is first nul then set accordingly and return
		if(inode_table[inodeIndex].link_cnt == 0){ 
			availableInode = i; 
			return;
		}
	}
	
	//at this point no null values available so it is full
	availabInode = MAX_INODES; //have conditions taht will cehck this bfore use
}

void findOpenDir(int number){
	int i = number;
	for(i = number; i < MAX_INODES; i++){
		if(root_dir[i].file_name == 0){
			availableDir = i;
			return;
		}
	}
	
	//out of space
	availableDir = MAX_INODES; //have conditions that will check this	
}

void findOpenFd(int number){
	int i = number;
	for(i = number; i < MAX_FILES; i++){
		if(fd_table[i].inode_number == 0){
			availableFd = i;
			return;
		}
	}
	
	//out of space
	availableDir = MAX_FILES; //have conditions that will check this
}	

void findCurrentBlock(int number){
	int i;
	for(i = number; i < MAX_BLOCKS; i++){
		//search until find free block
		if(free_blocks[i] == 0){
			currentBlock = i; //set what the current open block is
			return;
		}
	}

	//out of space
	currentBlock = MAX_BLOCKS; //have conditions that will check this	

			currentBlock = i;
		

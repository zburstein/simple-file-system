#include "sfs_api.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 512
#define MAX_BLOCKS 500

#define DISK_FILE "sfs_disk.disk"
#define MAX_INODES 50 //not sure what this should be
#define MAX_FILES 50 //not sure wha thtis should be

void zeroDisk();
void init_sb();
void add_root_dir_inode();
void init_free_blocks();
int findInDir(const char *path);
int findInFd (unsigned int inodeNumber);
void findOpenInode(int number);
void findOpenDir(int number);
void findOpenFd(int number);
void findCurrentBlock(int number);

super_block_t sb; //the super block
dir_entry_t root_dir[MAX_FILES]; //array of directory entris will be the directory structure
inode_t inode_table[MAX_INODES]; //the inode table will be array of inodes
fd_table_entry_t fd_table[MAX_FILES]; //the file descriptor table
unsigned short free_blocks[MAX_BLOCKS]; //free blocks jsut an int array with each index mapped to corresponding block
int currentBlock; //to know what block to write the data to
int availableInode; //will represent the lowest value inode space available
int availableDirectory; //should i use this
int availableFd = 0;
int getNextFileLocation = 0;
int num_inode_blocks;
int dirStartBlock;
int freeBlocksLength;

//rot directory inode: mode? size? ptrs?
//does root directory have an entry for itself within itself?
//instead of using current block should i just use the freeblocks array? 
//what if we run out of inode or directory sapce in block?
//in conjunction with previous question what if we alot and then it happens to be bigger than that
//are those that i declare to keep track of entries in fd table bad practice? 
//he created close as having int return but not that in the assignment. So which is it? What should failure and success codes be
//do we have to malloc a space for the buffer. I dont understand when we are supposed to be doing this
//also whats the deal with caching
//need to figure out data ptrs and what to do when there is more tan one block of memory being used. might just be aloop. but dont really know how data ptrs work so cant implement it
//Check the usages of the available stuff to ensure that I have the properc checks before using them
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
//also what about the false flag
//a little confused about get next file name too. 
//what to return for a failed open
//do i have to malloc block content array for
//if write too big shoudl we write to certain point that there is no logner any space or check first and not allow any write

void mksfs(int fresh) {
	//Implement mksfs here

	if(fresh == 1){
		init_fresh_disk(DISK_FILE, BLOCK_SIZE, MAX_BLOCKS);//initializes the disk of given name and of the size
		//zeroDisk(); //zero all contents 

		//allot space to dir and inode based upon how mnay coulf theoretically end up fitting. so will have to do sizeof / BLOCKSIZE
		//create superblock and write it to disk
		init_sb(); //initialize the super block
		write_blocks(0, 1, &sb); //write to block 0 the contents of sb		
		currentBlock = 1;

		//initialize inode table
		add_root_dir_inode(); //create root inode
		num_inode_blocks = (sizeof(inode_t) * MAX_INODES) / BLOCK_SIZE + 1; //take down its length
		write_blocks(currentBlock, num_inode_blocks, &inode_table); //write it disk
		availableInode = 1; //one inode used
		currentBlock += num_inode_blocks; //adjust waht block is free

		//create and write root dir
		dirStartBlock = currentBlock;//keep track of its start block
		write_blocks(currentBlock, inode_table[0].size, &root_dir);//write at the block with the length
		availableDirectory = 0; //set available Directory
		currentBlock += inode_table[0].size; //adjust current block

		//init the free blocks array and write it
		init_free_blocks();
		freeBlocksLength = sizeof(free_blocks) / BLOCK_SIZE + 1;
		write_blocks(MAX_BLOCKS - freeBlocksLength, freeBlocksLength, &free_blocks);//now write it to
	}
	else {
		////?????
		//read the root directory, superblock, and inode table into memory(the global variables)
		//will also need to properly initialize the global variables i use
		//read_blocks(1, )
	}
	return;
}

int sfs_getnextfilename(char *fname) {
	//Implement sfs_getnextfilename here	

	return 0;
}


int sfs_getfilesize(const char* path) {
	//Implement sfs_getfilesize here	
	int index = findInDir(path);
	int inodeNumber = root_dir[index].inode;
	if(index >= 0)
		return inode_table[inodeNumber].size;
	else{
		printf("File not found\n");
		return -1;
	}	
	
}

int sfs_fopen(char *name) {
	//Implement sfs_fopen here	
	int fd, index, i;
	
	//find its index in the directory
	index = findInDir(name);

	//if it does not exist then we need to create it by creating an inode and placig it in hte inode table as well as placing it in the directory
	if(index == -1){ //does not exist so need to create the file

		printf("inode: %d     block:%d    dir:%d      fd: %d\n", availableInode, currentBlock, availableDirectory, availableFd);
	
		//first check if space in inode table
		if(availableInode >= MAX_INODES || currentBlock >= MAX_BLOCKS || availableDirectory >= MAX_INODES || availableFd >= MAX_FILES){
			printf("There is no space available for another file\n");
			return -1;
		}

		//else place new inode
		inode_table[availableInode].mode = 0x755;
		inode_table[availableInode].link_cnt = 1;
		inode_table[availableInode].uid = 0;
		inode_table[availableInode].gid = 0;
		inode_table[availableInode].size = 0;
		for(i = 0; i < 12; i++){
			inode_table[availableInode].data_ptrs[i] = 0; //do not allot a spot yet
		}

		write_blocks(1, num_inode_blocks, &inode_table); //write the updated inode table to disk

		//create new directory entry
		if(strlen(name) > 20 || strlen(strchr(name, '.')) > 4){ 
			printf("File name is invalid. Must not be longer than 20 char total and extension cannot be longer than 3 char\n");
			return -1;
		}
		strcpy(root_dir[availableDirectory].file_name, name); //copy the file name
		root_dir[availableDirectory].inode = availableInode;	//and the inodeNumber
		write_blocks(dirStartBlock, inode_table[0].size, &root_dir); //write the updated directory structure to disk
		index = availableDirectory; //set the index in directory
		
		//change the values of availableInode and availableDIr accordingly for next entry
		findOpenInode(availableInode);
		findOpenDir(availableDirectory); 
	}	

	//file already exists or has been created at this point

	//if already open
	if(findInFd(root_dir[index].inode) != -1){
		printf("File already open\n");
		return -1;
	}



	//need to give file an entry in the file descriptor table
	//if no space in fd table
	if(availableFd >= MAX_FILES){
		printf("No more space available\n");
		return -1;
	}
	

	//wset all the properties in fdtable accordingly
	fd_table[availableFd].inode_number = root_dir[index].inode; //in fd table set the inode number
	fd_table[availableFd].rw_pointer = inode_table[root_dir[index].inode].size; //set read write pointer to end of file
	

	fd = availableFd; //set the file descriptor index
	findOpenFd(availableFd);//update availe fd index

	return fd;//return file descriptor for the file opened
}

int sfs_fclose(int fileID){	
	
	//first check if present in table
	if(fd_table[fileID].inode_number == 0){
		printf("There is no file with give handle\n");
		return 0;
	}

	//else remove the file from the fd table
	fd_table[fileID].inode_number = 0;
	fd_table[fileID].rw_pointer = 0;

	//if the file descriptor being moved is the lowest value now open in fd_table
	if(fileID < availableFd)
		availableFd = fileID;

	return 1;
}
/*

int sfs_fread(int fileID, char *buf, int length){
	//find reference within the file descriptor table and get the inode
	int inodeNumber, block_number, loc_in_block, bytesRead = 0;
	char blockContent[BLOCK_SIZE];
	inodeNumber = fd_table[fileID].inode_number;	
	
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
	if(fd_table[fileID].rw_pointer + length > inode_table[inodeNumber].size)
		length = inode_table[inodeNumber].size - fd_table[fileID].rw_pointer;

	//read the data. need loop in the event taht read spans multiple blocks
	while(bytesRead < length){
		//get the block that the r/w pointer is on and where within the block it is
		block_number = inode_table[inodeNumber].rw_pointer / BLOCK_SIZE; 
		loc_in_block = inode_table[inodeNumber].rw_pointer % BLOCK_SIZE;

		//if need to use indirect pointer
		if(block_number > 11){
			//do some thing
		}
		//if using a direct pointer
		else{
			//read the block into a buffer
			read_blocks(inode_table[inodeNumber].data_ptrs[block_number], 1, &blockContent[0]);
			
			//copy into provided buffer. Need to determine though the length to be copied
			//if rest of read will be contained within current block
			if(length + loc_in_block < BLOCK_SIZE){
				memcpy(&buf[bytesRead], &blockContent[loc_in_block], length); //will not go outside of the boundary of blockContent
				bytesRead += length; //update the number of bytes read
				length -= length; //update remaining length
				fd_table[inodeNumber].rw_pointer += length; //update pointer
			}
			//if read will have to span to next block
			else{
				memcpy(&buf[bytesRead], &blockContent[loc_in_block], BLOCK_SIZE - loc_in_block);//copy until the end of the block
				bytesRead += BLOCK_SIZE - loc_in_block; //update number of bytes read
				length -= BLOCK_SIZE - loc_in_block; //update the remaining length
				fd_table[inodeNumber].rw_pointer += BLOCK_SIZE -loc_in_block; //update teh read write pointer
			}
		}

	}

	return bytesRead; //return the number of characters copied
}
*/



int sfs_fwrite(int fileID, const char *buf, int length){
	//Implement sfs_fwrite here	
	int inodeNumber, block_number, loc_in_block, bytesWritten = 0, i, currentIndirectPointer, workingBlock;
	char *blockContent;	
	unsigned int *indirectBlock;

	blockContent = (char*) malloc(BLOCK_SIZE); //make new buffer
	indirectBlock = (unsigned int*) malloc(BLOCK_SIZE / sizeof(unsigned int)); /////////////////need to fix this

	//get inode number
	inodeNumber = fd_table[fileID].inode_number;

	//if bad handle
	if(inodeNumber == 0){
		printf("File descriptor does not exist\n");
		return 0;
	}

	//if the file is empty, need to allot it a block
	if(inode_table[inodeNumber].size == 0){
		//if not enough space for file
		if(currentBlock >= MAX_BLOCKS){
			printf("No space available\n");
			return 0;
		}

		printf("Has no block yet. Giving it %d\n", currentBlock);

		//give it a block and adjust block values accordingly
		inode_table[inodeNumber].data_ptrs[0] = currentBlock;
		free_blocks[currentBlock] = 1;
		findCurrentBlock(currentBlock);
	}

	while(length > 0){
		//get the block that the r/w pointer is on and where within the block it is
		block_number = fd_table[fileID].rw_pointer / BLOCK_SIZE; 
		loc_in_block = fd_table[fileID].rw_pointer % BLOCK_SIZE;

		printf("current block is %d and the block number is %d and its ptr is %d. The rw pointer is %d\n", currentBlock, block_number, inode_table[inodeNumber].data_ptrs[block_number], fd_table[fileID].rw_pointer);

		//if need a new block
		//still have available data ptrs
		if(block_number < 12 && inode_table[inodeNumber].data_ptrs[block_number] == 0){
			//if no space
			if(currentBlock >= MAX_BLOCKS){
				printf("No more space available to complete write of file\n");
				break;
			}
			else{
				printf("Giving new block \n");
				inode_table[inodeNumber].data_ptrs[block_number] = currentBlock; //assign it block
				free_blocks[currentBlock] = 1; //update free blocks bit map
				findCurrentBlock(currentBlock); //get new current block
			}
		}
		//if first use fo indirect pointer
		else if(block_number == 12 && inode_table[inodeNumber].indirect_pointer == 0){
			//need to give it a block
			//if no space
			printf("allotting indirect pointer to %d", currentBlock);
			if(currentBlock >= MAX_BLOCKS){
				printf("No more space available to complete write of file\n");
				break;
			}
			else{
				inode_table[inodeNumber].indirect_pointer = currentBlock; //set the indirect pointer
				free_blocks[currentBlock] = 1; //update free blocks
				findCurrentBlock(currentBlock); //find enxt available block
				printf("will then assign pointer %d\n", currentBlock);
				indirectBlock[0] = currentBlock; //then set it in array we will use to write to block
				currentIndirectPointer = currentBlock; //also keep it in easier access fo use in write
				write_blocks(inode_table[inodeNumber].indirect_pointer, 1, &indirectBlock); //write into the indeirect pointer the new block
				free_blocks[currentBlock] = 1;//set in free blocks array
				findCurrentBlock(currentBlock);//find next available block
			}
		}
		else if(block_number > 12 && inode_table[inodeNumber].indirect_pointer != 0){ //means we are in the indirect pointer adn it already exists
			//maybe place below else within this if statement so that this conditional determins that indirect pointer already exists
			//then checks on what block number it would be on. if within indirect pointer block there is a value, then can read from that and write to thath block
			//if it is 0 value then need to allot new block and write tot that (the conditional below)
			read_blocks(inode_table[inodeNumber].indirect_pointer, 1, &indirectBlock); //read the contents fo the block

			//if there is block already allotted 
			if(indirectBlock[block_number - 12] != 0){
				currentIndirectPointer = indirectBlock[block_number - 12]; //this will be the block that will be worked with
			}
			else{
				//if no spaceon disk
				if(currentBlock >= MAX_BLOCKS){
					printf("No more space available to complete write of file\n");
					break;
				}
				//if no more pointers available
				else if(block_number > 12 + (BLOCK_SIZE / sizeof(unsigned int))){
					printf("No more poointers avaialble to the file. You have reached max file size\n");
				}

				read_blocks(inode_table[inodeNumber].indirect_pointer, 1, &indirectBlock);//read the block

				printf("Assigning new indirect pointer at %d\n", currentBlock);
				
				//find next empty pointer in the block
				for(i=0; i < BLOCK_SIZE / sizeof(unsigned int); i++){
					printf("we are looping. loop until %u. value at location is %u. The length of indirectBlock is %u bytes and each one is %u\n", sizeof(indirectBlock) / sizeof(indirectBlock[0]), indirectBlock[i], sizeof(indirectBlock), sizeof(indirectBlock[0]));
					if(indirectBlock[i] == 0){
						printf("%d indirect pointers have already been assigned. Now assigning %d\n", i, i+1);
						indirectBlock[i] = currentBlock; //set the next pointer
						currentIndirectPointer = currentBlock; //set it to easier access variable
						write_blocks(inode_table[inodeNumber].indirect_pointer, 1, &indirectBlock); //then update the indirect pointer
						free_blocks[currentBlock] = 1; //update free blocks
						findCurrentBlock(currentBlock); //find next available block
						break;
					}
				}
			}
		}



		//then begin write by reading block
		//if on indirect pointer
		if(block_number > 11){
			read_blocks(currentIndirectPointer, 1, &blockContent[0]); //read the block
			workingBlock = currentIndirectPointer;
		}
		//if direct pointer
		else{
			//read the block into buffer
			read_blocks(inode_table[inodeNumber].data_ptrs[block_number], 1, &blockContent[0]);
			workingBlock = inode_table[inodeNumber].data_ptrs[block_number];
		}


		//if write will finish within this block
		if(length + loc_in_block < BLOCK_SIZE){
			printf("Will finish within block and writing to %d\n", workingBlock);
			//printf("The loc in block is %d", loc_in_block);
			memcpy(&blockContent[loc_in_block], &buf[bytesWritten], length);
			//write_blocks(inode_table[inodeNumber].data_ptrs[block_number], 1, &blockContent[0]); //write it
			write_blocks(workingBlock, 1, &blockContent[0]); //write it
			bytesWritten += length; //update number of byteswritten
			length -= length; //update the length
			fd_table[fileID].rw_pointer += length;//update the pointer location
		}

		//if will spand more than one block
		else{
			printf("Takes more than one block and writing to %d\n", workingBlock);
			memcpy(&blockContent[loc_in_block], &buf[bytesWritten], BLOCK_SIZE - loc_in_block);//copy until the end of the block
			//write_blocks(inode_table[inodeNumber].data_ptrs[block_number], 1, &blockContent[0]);//write the block
			write_blocks(workingBlock, 1, &blockContent[0]);//write the block
			bytesWritten += BLOCK_SIZE - loc_in_block; //update bytes written
			length -= BLOCK_SIZE -loc_in_block; //update length
			fd_table[fileID].rw_pointer += BLOCK_SIZE - loc_in_block; //update the rw pointer
		}

		//adjust size if needed
		if(fd_table[fileID].rw_pointer > inode_table[inodeNumber].size){
			inode_table[inodeNumber].size = fd_table[fileID].rw_pointer;
		}
			//and then loop back up again
	}

	write_blocks(1, num_inode_blocks, &inode_table); //update inode table on block
	write_blocks(MAX_BLOCKS - freeBlocksLength, freeBlocksLength, &free_blocks); //then update free blocks
	free(blockContent);//free the buffer created
	free(indirectBlock);

	return bytesWritten;
}

/*
int sfs_fseek(int fileID, int loc){
	//Implement sfs_fseek here	
	int inodeNumber;

	//get inode number
	inodeNumber = fd_table[fileID].inode_number;  

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
	root_dir[dirIndex].file_name = "\0";
	root_dir[dirIndex].inode = 0;

	//reset direntry if needed
	if(availableDirectory > dirIndex){
		availableDirectory = dirIndex;
	}
	
	return 0;
}

*/

void init_sb(){
	sb.magic = 1234;
	sb.block_size = BLOCK_SIZE;
	sb.fs_size = MAX_BLOCKS * BLOCK_SIZE;
	sb.inode_table_length = MAX_INODES;
	sb.root_dir_inode = 0; //why is this 0
}

void zeroDisk(){
	//some of this seems off like the directory. Also why no zeroing of the data portion
	bzero(&sb, sizeof(super_block_t)); //zero the super block
	bzero(&inode_table[0], sizeof(inode_t) * MAX_INODES); //inode table
	bzero(&root_dir, sizeof(dir_entry_t)); //root directory
	bzero(&free_blocks[0], sizeof(unsigned short) * MAX_BLOCKS); //free blocks array
	bzero(&fd_table[0], sizeof(fd_table_entry_t) * MAX_BLOCKS); //fd table
}

void add_root_dir_inode(){
	inode_table[0].mode = 0x755; //why this?
	inode_table[0].link_cnt = 1;
	inode_table[0].uid = 0; //we dont have users
	inode_table[0].gid = 0; //dont have groups
	inode_table[0].size = (sizeof(dir_entry_t) * MAX_FILES) / BLOCK_SIZE + 1; //making this the number of blocks
	inode_table[0].data_ptrs[0] = num_inode_blocks + 1; //store in the blcok after the superblock and inode table
}

void init_free_blocks(){
	//mark the blocks tahta are used
	int i;
	for(i = 0; i < currentBlock; i++){
		free_blocks[i] = 1; //set it equal to 1 to indicate it is taken
	}

	for(i = MAX_BLOCKS - freeBlocksLength; i < MAX_BLOCKS; i ++){
		free_blocks[i] = 1;
	}
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


int findInFd(unsigned int inodeNumber){
	int i;
	//search through fd table if find number
	for(i = 0; i < MAX_FILES; i++){
		if(fd_table[i].inode_number == inodeNumber){
			return i;
		}
		else if(fd_table[i].inode_number == 0){
			return -1;
		}
	}
	return -1;
}

	
void findOpenInode(int number){
	int i = number;
	for(i = number; i < MAX_INODES; i++){//can start at the current location
		//if it is first nul then set accordingly and return
		if(inode_table[i].link_cnt == 0){ 
			availableInode = i; 
			return;
		}
	}
	
	//at this point no null values available so it is full
	availableInode = MAX_INODES; //have conditions taht will cehck this bfore use
}

void findOpenDir(int number){
	int i = number;
	for(i = number; i < MAX_INODES; i++){
		if(root_dir[i].file_name[0] == '\0'){
			availableDirectory = i;
			return;
		}
	}
	
	//out of space
	availableDirectory = MAX_INODES; //have conditions that will check this	
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
	availableDirectory = MAX_FILES; //have conditions that will check this
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
}
		

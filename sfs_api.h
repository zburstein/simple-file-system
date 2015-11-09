
#define MAXFILENAME 60 //does this need ot be changed?

void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fseek(int fileID, int loc);
int sfs_remove(char *file);

typedef struct super_block{
	unsigned int magic; //magic number
	unsigned int block_size; //the size of each block
	unsigned int fs_size; //size of each block * number of blocks
	unsigned int inode_table_length; //Number of blocks that it occupies
	unsigned int root_dir_inode; //pointer to the root directory inode
} super_block_t;

typedef struct inode{
	unsigned int mode; //Mode that its opened in?????? file type
	unsigned int link_cnt; //????Not sure what this is. how many hard links point to i node
	unsigned int uid; //user id
	unsigned int gid; //group id
	unsigned int size; //size fo the file
	unsigned int data_ptrs[10]; //pointers to the disk blocks that store the files contents
} inode_t;

typedef struct dir_entry{
	const char file_name[20]; //the name
	unsigned int inode; //the pointer to location in inode table
} dir_entry_t;

typedef struct fd_table_entry{
	unsigned int inode_number; //pointer or number
	unsigned int rw_pointer;
	//unsigned int write_pointer;
	//unsigned int locpointer;
} fd_table_entry_t;

//do i need one for directory, inode table, and 

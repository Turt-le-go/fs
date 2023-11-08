#ifndef FS_H_
#define FS_H_

#define BLOCK_SIZE 8
#define FILE_NAME_SIZE 32

#define REGULAR_FILE_TYPE 'R'
#define DIRECTORY_FILE_TYPE 'D'

typedef struct inode {
	unsigned int size;
	void** blocks;
} inode;

typedef struct file {
	char type;
	char name[FILE_NAME_SIZE];
	inode* inode;
} file;

inode * _create_inode();
file * _create_file(const char * name, char type);
file * _get_file_from_name(const char * name);

void _add_file_to_dir(file * parent_dir_ptr, file * new_file_ptr);
unsigned char _remove_file_from_dir(file * parent_dir_ptr, file * file_ptr);

unsigned char _check_if_file_exists(const char * name);

//unsigned char _free_file(file * file_ptr);
unsigned char _free_inode(inode * inode_ptr, char type);

unsigned char mkfs();
unsigned char mkfs_free();

unsigned char mkdir(const char * name);
unsigned char cd(const char * name);

unsigned char stat(const char * name);
unsigned char ls(); 
unsigned char create(const char * name);
unsigned char link(const char * src, const char * dst);
unsigned char unlink(const char * name);

unsigned char mkdir(const char * name);
unsigned char rmdir(const char * name);

#endif

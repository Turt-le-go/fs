#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "fs.h"

file * _current_dir;
file * _root_dir;
inode _null_inode = {0};

unsigned char mkfs(){
	_root_dir = _create_file("/",'D');
	*((file**)_root_dir->inode->blocks) = _root_dir;

	_current_dir = _root_dir;

	return 0;
}

unsigned char mkfs_free(){
	_free_inode(_root_dir->inode, _root_dir->type);
	free(_root_dir);
	return 0;
}

unsigned char create(const char * name){
	unsigned int err;
	if ((err = _check_if_file_exists(name))){
		return err;
	}
	file * file_ptr = _create_file(name, REGULAR_FILE_TYPE);
	char * block = (char*) malloc(BLOCK_SIZE);
	*block = '\0';
	*((char**)file_ptr->inode->blocks) = block;
	_add_file_to_dir(_current_dir, file_ptr);
	return 0;
}

unsigned char mkdir(const char * name){
	unsigned int err;
	if ((err = _check_if_file_exists(name))){
		return err;
	}
	file * dir_ptr = _create_file(name, DIRECTORY_FILE_TYPE);
	*(dir_ptr->inode->blocks) = _current_dir;
	_add_file_to_dir(_current_dir, dir_ptr);
	return 0;
}

unsigned char ls(){
	printf("\n");
	printf("%p %c %s\n", (void*)_current_dir->inode, _current_dir->type, ".");
	file* parent_dir_ptr = *((file**)_current_dir->inode->blocks);
	printf("%p %c %s\n", (void*)parent_dir_ptr->inode, parent_dir_ptr->type, "..");
	for (unsigned int i = 1; i<_current_dir->inode->size; i++){
		file* file_ptr = *(((file**)_current_dir->inode->blocks)+i);
		printf("%p %c %s\n", (void*)file_ptr->inode, file_ptr->type, file_ptr->name);
	}
	return 0;
}

unsigned char stat(const char * name){
	file * file_ptr;
	if ((file_ptr = _get_file_from_name(name)) == NULL){
		fprintf(stderr, "Can't find file - %s\n", name);
		return ENOENT;
	}
	printf("\n");
	printf("File: %s;\n", file_ptr->name);
	printf("Type: %c;\n", file_ptr->type);
	printf("Inode: %p;\n", (void*)file_ptr->inode);
	printf("Inode blocks size: %u;\n", file_ptr->inode->size);
	return 0;
}

unsigned char cd(const char * name){
	if (!strcmp(name, ".")){
		return 0;
	}else if (!strcmp(name, "..")){
		_current_dir = *((file**)_current_dir->inode->blocks);
		return 0;
	}else if (!strcmp(name, "/")){
		_current_dir = _root_dir;
		return 0;
	}
	for (unsigned int i = 1; i<_current_dir->inode->size; i++){
		file* file_ptr = *(((file**)_current_dir->inode->blocks)+i);
		if (!strcmp(name, file_ptr->name)){
			if (file_ptr->type == DIRECTORY_FILE_TYPE){
				_current_dir = file_ptr;
				return 0;
			}
			fprintf(stderr, "%s - is not a directory\n", name);
			return ENOTDIR;
		}
	}
	fprintf(stderr, "Can't find directory - %s\n", name);
	return ENOENT;
}

unsigned char link(const char * src, const char * dst){
	file * src_file_ptr;
	if ((src_file_ptr = _get_file_from_name(src)) == NULL){
		fprintf(stderr, "Can't find file - %s\n", src);
		return ENOENT;
	}
	file * dst_file_ptr = _create_file(dst, src_file_ptr->type);
	_free_inode(dst_file_ptr->inode, dst_file_ptr->type);
	dst_file_ptr->inode = src_file_ptr->inode;
	_add_file_to_dir(_current_dir, dst_file_ptr);
	return 0;
}

unsigned char unlink(const char * name){
	file * file_ptr;
	if ((file_ptr = _get_file_from_name(name)) == NULL){
		fprintf(stderr, "Can't find file - %s\n", name);
		return ENOENT;
	}
	// check if other files use this inode.
	inode * inode_ptr = file_ptr->inode;
	bool inode_is_used = false;
	for( unsigned int i = 1; i < _current_dir->inode->size; i++){
		file * tmp_file_ptr = *((file**)(_current_dir->inode->blocks+i));
		if (tmp_file_ptr->inode == inode_ptr && tmp_file_ptr != file_ptr){
			inode_is_used = true;
			break;
		}
	}
	if (!inode_is_used){
		_free_inode(inode_ptr, file_ptr->type);
		file_ptr->inode = NULL;
	}
	_remove_file_from_dir(_current_dir, file_ptr);
	free(file_ptr);
	return 0;
}

unsigned char rmdir(const char * name){
	return unlink(name);
}

unsigned char _remove_file_from_dir(file * parent_dir_ptr, file * file_ptr){
	parent_dir_ptr->inode->size--;
	file ** realloc_ptr = (file**)calloc(parent_dir_ptr->inode->size, BLOCK_SIZE);
	if (!realloc_ptr){
		perror("Can't reallocate memory for inode blocks\n");
		exit(errno);
	}
	file* tmp_file_ptr = NULL;
	unsigned int j = 0;
	for (unsigned int i = 0; i < parent_dir_ptr->inode->size+1; i++){
		tmp_file_ptr = *((file**)(parent_dir_ptr->inode->blocks+i));
		if (tmp_file_ptr == file_ptr) continue;
		*((file**)(realloc_ptr+j)) = tmp_file_ptr;
		j++;
	}
	free(parent_dir_ptr->inode->blocks);
	parent_dir_ptr->inode->blocks = (void**)realloc_ptr;
	return 0;
}

file * _get_file_from_name(const char * name){
	if (!strcmp(name, ".")){
		return _current_dir;
	}else if (!strcmp(name, "..")){
		return *((file**)_current_dir->inode->blocks);
	}else if (!strcmp(name, "/")){
		return _root_dir;
	}
	for (unsigned int i = 1; i<_current_dir->inode->size; i++){
		file* file_ptr = *(((file**)_current_dir->inode->blocks)+i);
		if (!strcmp(name, file_ptr->name)){
			return file_ptr;
		}
	}
	return NULL;
}

unsigned char _check_if_file_exists(const char * name){
	if (!strcmp(name, ".")){
		fprintf(stderr, "File \"%s\" exists.\n", name);	
		return EEXIST;
	}
	for (unsigned int i = 1; i<_current_dir->inode->size; i++){
		file* file_ptr = *(((file**)_current_dir->inode->blocks)+i);
		if (!strcmp(name, file_ptr->name)){
			fprintf(stderr, "File \"%s\" exists.\n", name);	
			return EEXIST;
		}
	}
	return 0;	
}

void _add_file_to_dir(file * parent_dir_ptr, file * new_file_ptr){
	parent_dir_ptr->inode->size++;
	file ** realloc_ptr = (file**)calloc(parent_dir_ptr->inode->size, BLOCK_SIZE);
	if (!realloc_ptr){
		perror("Can't reallocate memory for inode blocks\n");
		exit(errno);
	}
	file** tmp_file_ptr = NULL;
	for (unsigned int i = 0; i < parent_dir_ptr->inode->size-1; i++){
		tmp_file_ptr = realloc_ptr+i;
		*tmp_file_ptr = *((file**)(parent_dir_ptr->inode->blocks+i));
	}
	tmp_file_ptr = realloc_ptr+((parent_dir_ptr->inode->size-1));
	*tmp_file_ptr = new_file_ptr;
	free(parent_dir_ptr->inode->blocks);
	parent_dir_ptr->inode->blocks = (void**)realloc_ptr;
}

unsigned char _free_inode(inode * inode_ptr, char type){
	if (type == DIRECTORY_FILE_TYPE && *((file**)inode_ptr->blocks)){		
		(*((file**)inode_ptr->blocks)) = NULL;
		//find unique inodes
		inode * unique_inodes[inode_ptr->size];
		char unique_inodes_type[inode_ptr->size];
		memset(unique_inodes, 0, inode_ptr->size);
		unsigned int unique_inodes_num = 0;
		for(unsigned int i = 1; i < inode_ptr->size; i++){
			inode * tmp_inode_ptr = (*((file**)(inode_ptr->blocks+i)))->inode;
			char tmp_inode_type = (*((file**)(inode_ptr->blocks+i)))->type;
			(*((file**)(inode_ptr->blocks+i)))->inode = NULL;
			bool is_unique = true;
			for(unsigned int j = 0; j < unique_inodes_num; j++){
				if (tmp_inode_ptr == unique_inodes[j]){
					is_unique = false;
					break;
				}
			}
			if (is_unique){
				unique_inodes[unique_inodes_num] = tmp_inode_ptr;
				unique_inodes_type[unique_inodes_num] = tmp_inode_type;
				unique_inodes_num++;
			}
		}
		//free unique inodes
		for(unsigned int i = 0; i < unique_inodes_num; i++){
			_free_inode(unique_inodes[i], unique_inodes_type[i]);
		}
	}
	for(unsigned int i = 0; i < inode_ptr->size; i++){
		if (*(inode_ptr->blocks+i)){
			free(*(inode_ptr->blocks+i));
			*(inode_ptr->blocks+i) = NULL;
		}
	}
	free(inode_ptr->blocks);
	inode_ptr->blocks = NULL;
	free(inode_ptr);
	inode_ptr = NULL;
	return 0;
}

file * _create_file(const char * name, char type){
	file * file_ptr = (file *)malloc(sizeof(file));
	if (file_ptr == NULL){
		perror("Can't allocate memory for file");
		exit(errno);
	}
	file_ptr->type = type;
	strcpy(file_ptr->name,name);
	file_ptr->inode = _create_inode();

	return file_ptr;
}

inode * _create_inode(){
	inode * inode_ptr = (inode *)malloc(sizeof(inode));
	if (inode_ptr == NULL){
		perror("Can't allocate memory for inode");
		exit(errno);
	}
	inode_ptr->size = 1;
	inode_ptr->blocks = (void**) calloc(inode_ptr->size,BLOCK_SIZE);
	if (inode_ptr->blocks == NULL){
		perror("Can't allocate memory for inode blocks");
		exit(errno);
	}
	return inode_ptr;
}

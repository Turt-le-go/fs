#include <stdio.h>
#include "fs.h"


int main(){

	mkfs();
	create("test1");
	create("test2");
	mkdir("ttt");
	ls();
	cd("ttt");
	create("file1");
	create("file1");
	ls();
	cd("..");
	ls();
	link("ttt", "aaa");
	stat("ttt");
	stat("aaa");
	unlink("test1");
	ls();
	rmdir("ttt");
	ls();
	cd("aaa");
	ls();
	cd("..");
	rmdir("aaa");
	ls();
	mkfs_free();

	return 0;
}

/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.

	gcc -Wall `pkg-config fuse --cflags --libs` cs1550.c -o cs1550
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define	MAX_FILES_IN_DIR (BLOCK_SIZE - (MAX_FILENAME + 1) - sizeof(int)) / \
	((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE - sizeof(unsigned long))

//512 * 3
//512 for the block size, 3 because the bitmap is the last 3 blocks of the .disk file
#define BITMAP_SIZE 512 * 20

	int getDirectory(int index);
	int findDirectory(char *directory);
	int getFile(int index);
	size_t findFile(char *file, char *ext, int dirIndex);
	int setBitmap(char *path, char value);
	int createDirectory(char *path, int ind);


	struct cs1550_directory_entry
	{
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_directory_entry cs1550_directory_entry;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

struct cs1550_disk_block
{
	//The first 4 bytes will be the value 0xF113DA7A 
	unsigned long magic_number;
	//And all the rest of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

typedef struct cs1550_disk_block cs1550_disk_block;

//How many pointers in an inode?
#define NUM_POINTERS_IN_INODE (BLOCK_SIZE - sizeof(unsigned int) - sizeof(unsigned long))/sizeof(unsigned long)

struct cs1550_inode
{
	//The first 4 bytes will be the value 0xFFFFFFFF
	unsigned long magic_number;
	//The number of children this node has (either other inodes or data blocks)
	unsigned int children;
	//An array of disk pointers to child nodes (either other inodes or data)
	unsigned long pointers[NUM_POINTERS_IN_INODE];
};

typedef struct cs1550_inode cs1550_inode;

//returns the index of the directory in the root struct array
int findDirectory(char *directory) {
	// printf("FIND DIRECTORY\n");
	//Initialize the return value to -1 to indicate that the directory has not been found yet
	int returnVal = -1;
	cs1550_root_directory *rootStruct = calloc(1, sizeof(*rootStruct));
	FILE *file = fopen(".disk", "r+b");
	fseek(file, 0, SEEK_SET);
	fread(rootStruct, sizeof(cs1550_root_directory), 1, file);
	fclose(file);

	int i;
	for (i = 0; i < rootStruct->nDirectories; i++) {
		//Set the struct at the current index in the .disk structure
		// printf("For loop index: %d | %s\n", i, rootStruct->directories[i].dname);
		//The directory matches the current string in the struct
		if (strcmp(directory, rootStruct->directories[i].dname) == 0) {
			returnVal = i;
			break;
		}
	}
	
	free(rootStruct);
	return returnVal;
}


//Returns the size of the file is located at in the subdirectory array
//dirIndex is the index in the root struct array directories that the directory is stored at
size_t findFile(char *filename, char *ext, int dirIndex) {
	// printf("FIND FILE\n");
	size_t returnVal = -1;
	int i = 0;

	//Read in the root struct
	FILE *file = fopen(".disk", "r+b");
	cs1550_root_directory *root = calloc(1, sizeof(*root));
	fread(root, sizeof(cs1550_root_directory), 1, file);

	//Get the start block of the directory and seek to it
	int startBlock = root->directories[dirIndex].nStartBlock;
	fseek(file, startBlock * sizeof(cs1550_root_directory), SEEK_SET);

	cs1550_directory_entry *directoryStruct = calloc(1, sizeof(*directoryStruct));
	fread(directoryStruct, sizeof(cs1550_directory_entry), 1, file);
	fclose(file);

	//Search through the files in the directory struct array
	for (i = 0; i < directoryStruct->nFiles; i++) {
		// printf("For loop index: %d | %s + %s\n", i, directoryStruct->files[i].fname, directoryStruct->files[i].fext);
		//If the element in the file array has the same filename and extension -> return the size of the file
		if ( (strcmp(filename, directoryStruct->files[i].fname) == 0) && (strcmp(ext, directoryStruct->files[i].fext) == 0) ) {
			returnVal = directoryStruct->files[i].fsize;
			break;
		}
	}

	free(directoryStruct);
	free(root);
	
	//Return -1 if the file is not found
	//Return the size of the file if it is found
	return returnVal;
}

int getFileIndex(char *filename, char *ext, cs1550_directory_entry *dir) {
	// printf("GETFILEINDEX\n");
	int index = -1;
	int i;
	for (i = 0; i < dir->nFiles; i++) {
		if ( (strcmp(filename, dir->files[i].fname) == 0) && (strcmp(ext, dir->files[i].fext) == 0) ) {
			index = i;
			break;
		}
	}

	return index;
}

//getattr
/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not. 
 *
 * man -s 2 stat will show the fields of a stat structure
 */
 static int cs1550_getattr(const char *path, struct stat *stbuf)
 {
 	int res = 0;
 	// printf("GETATTR\n");
 	memset(stbuf, 0, sizeof(*stbuf));

	//is path the root dir?
 	if (strcmp(path, "/") == 0) {
 		stbuf->st_mode = S_IFDIR | 0755;
 		stbuf->st_nlink = 2;
 	} 
	//Not a root
 	else {
 		
 		char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 		char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 		char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));
 		sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);


		//Therefore load block 0 into mem as a cs1550_root_directory struct and check its array
		//for the path string
		//directory is not empty

 		if (directory[0] != '\0' && filename[0] == '\0') {
 			// printf("Directory - %s\n", directory);
			//Find the directory in the path
			//If we actually found the directory, return the appropriate things
 			int findDirReturn = findDirectory(directory);
 			// printf("findDirectory returns %d\n", findDirReturn);
 			if (findDirReturn != -1) {
 				stbuf->st_mode = S_IFDIR | 0755;
 				stbuf->st_nlink = 2;
				res = 0; //no error
			}
			else {
				// printf("Directory does not exist\n");
				res = -ENOENT;
			}

		}

		//load the block of mem that the subdirectory is in into mem as a cs1550_entry
			//check its array is there is a file name with the correct extension
		//Check if name is a regular file
		else if (filename[0] != '\0') {
			// printf("File - %s.%s\n", filename, extension);
			int directoryIndex = findDirectory(directory);
			size_t size = findFile(filename, extension, directoryIndex);
			// printf("File size - %d\n", size);
			if (size != -1) {
				//regular file, probably want to be read and write
				stbuf->st_mode = S_IFREG | 0666; 
				stbuf->st_nlink = 1; //file links
				stbuf->st_size = size; //file size - make sure you replace with real size!
				res = 0; // no error
			}
			else {
				// printf("File does not exist\n");
				res = -ENOENT;
			}
		}
		//Free the things we sscanf'd
		free(directory);
		free(filename);
		free(extension);
	}

	
	return res;
}

//readdir
/* 
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
 static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
 	off_t offset, struct fuse_file_info *fi)
 {
 	// printf("READDIR\n");
	//Since we're building with -Wall (all warnings reported) we need
	//to "use" every parameter, so let's just cast them to void to
	//satisfy the compiler
 	(void) offset;
 	(void) fi;

 	int returnVal = 0;

 	//Read in the root
 	cs1550_root_directory *root = calloc(1, sizeof(*root));
 	FILE *file = fopen(".disk", "r+b");
 	fread(root, sizeof(*root), 1, file);
 	fclose(file);

 	if (strcmp(path, "/") == 0) {
 		int i;
 		for (i = 0; i < root->nDirectories; i++) {
 			printf("%s\n", root->directories[i].dname);
 			filler(buf, root->directories[i].dname, NULL, 0);
 		}
 	}

 	//If the path is not a root, load block 0 into mem as a struct cs1550_root_directory
		//Check its array for the directory in path
 	else {
 		//Load block into memory
 		char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 		char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 		char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));
 		sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 		if (filename[0] != '\0')
 			returnVal = -ENOENT;

	 	//findDirectory will return the index of the directory in the root struct array of directories
 		int findDirReturn = findDirectory(directory);
 		//If we found the directory
 		if (findDirReturn != -1) {

 			//Get the start block of the directory
 			long nStartBlock = root->directories[findDirReturn].nStartBlock;
 			//Open the file and seek to the appropriate location in the .disk file
 			file = fopen(".disk", "r+b");
 			fseek(file, sizeof(cs1550_directory_entry) * nStartBlock, SEEK_SET);

 			//Allocate for the directory struct and read it from the file
 			cs1550_directory_entry *dir = calloc(1, sizeof(*dir));
 			fread(dir, sizeof(*dir), 1, file);
 			fclose(file);

 			int i;
 			for (i = 0; i < dir->nFiles; i++) {
 				char fileBuf[MAX_FILES_IN_DIR + MAX_EXTENSION + 5];
 				strcpy(fileBuf, dir->files[i].fname);
 				strcat(fileBuf, ".");
 				strcat(fileBuf, dir->files[i].fext);
 				filler(buf, fileBuf, NULL, 0);
			    // filler(buf, dir->files[i].fname, NULL, 0);
 			}

 			free(dir);
 		}
 		//Else we couldn't find the directory
 		else {
 			// printf("Directory not found\n");
 			returnVal = -ENOENT;
 		}
 		free(directory);
 		free(filename);
 		free(extension);
 	}

 	//the filler function allows us to add entries to the listing
	//read the fuse.h file for a description (in the ../include dir)
 	filler(buf, ".", NULL, 0);
 	filler(buf, "..", NULL, 0);

 	free(root);

 	return returnVal;
 }

//value is either 0 or 1
//Returns the free index in the bit map/the block # that the disk block will be stored at in the file
 int setBitmap(char *filename, char value) {
 	// printf("SETBITMAP\n");

 	int returnVal = -1;
 	FILE *file = fopen(".disk", "r+b");
 	if (file == NULL) {
 		fclose(file);
 		// printf("Error on fopen");
 		return -1;
 	}
	//Go to the start of the bitmap in the .disk file
 	fseek(file, -BITMAP_SIZE, SEEK_END);
	//Read the bitmap into a char array
 	char *bitmap;
 	bitmap = malloc(sizeof(bitmap) * BITMAP_SIZE);
 	fread(bitmap, sizeof(char), BITMAP_SIZE, file);

	//Need to search through the bitmap to find a free space
 	int i;
 	for (i = 1; i < BITMAP_SIZE; i++) {
 		if ( (bitmap[i] == '\0' || bitmap[i] == '0') && value == '1') {
 			bitmap[i] = value;
 			returnVal = i;
 			break;
 		}
 	}
 	fseek(file, -BITMAP_SIZE, SEEK_END);
 	fwrite(bitmap, sizeof(char), BITMAP_SIZE, file);
 	fclose(file);

 	free(bitmap);

 	return returnVal;
 }

//Create directory
 int createDirectory(char *dir, int ind) {
 	// printf("createDirectory\n");

 	int returnVal = 0;
 	FILE *file = fopen(".disk", "r+b");
 	if (file == NULL) {
 		fclose(file);
 		printf("Error on fopen");
 		return -1;
 	}

 	cs1550_root_directory *rootDir = calloc(1, sizeof(*rootDir));
 	fseek(file, 0, SEEK_SET);
	//Read the .disk file
 	fread(rootDir, sizeof(*rootDir), 1, file);

	//Increment the total number of directories
 	rootDir->nDirectories += 1;

	//Copy the directory name into the root struct array of directories
 	strcpy(rootDir->directories[rootDir->nDirectories - 1].dname, dir);
 	// printf("Dir: %s\n", rootDir->directories[rootDir->nDirectories - 1].dname);
 	rootDir->directories[rootDir->nDirectories - 1].nStartBlock = ind;
 	// printf("Start block: %ld\n", rootDir->directories[rootDir->nDirectories - 1].nStartBlock);

 	// int i;
 	// for (i = 0; i < rootDir->nDirectories; i++)
 	// 	printf("[%d]: %s | %ld\n", i, rootDir->directories[i].dname, rootDir->directories[i].nStartBlock);

	//Seek back to beginning of the file
 	rewind(file);
	//Write the root struct back to the beginning of the file
 	fwrite(rootDir, sizeof(*rootDir), 1, file);

	//Seek to the block where the new directory should be made
 	fseek(file, sizeof(cs1550_directory_entry) * ind, SEEK_SET);
 	// printf("Seek to: %ld\n", sizeof(cs1550_directory_entry) * ind);

	//Allocate space for a directory, and set the number of files in that directory to 0
 	cs1550_directory_entry *newDir = calloc(1, sizeof(*newDir));
 	newDir->nFiles = 0;

	//Write the new directory
 	fwrite(newDir, sizeof(cs1550_directory_entry), 1, file);

 	fclose(file);
 	return returnVal;
 }

//mkdir
/* 
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */
 static int cs1550_mkdir(const char *path, mode_t mode)
 {
 	// printf("MKDIR\n");
 	//Else check if the subdirectory already exists
	//If it does, return an error
	//Else malloc a cs1550_directory_entry, then link this directory to the root
	//and write the updated bitmap (root block and directory block) back to the .disk file

 	(void) path;
 	(void) mode;

 	char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));

 	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 	// printf("Directory: %s\n", directory);

 	int returnVal = 0;

 	//Check if the length of the directory is greater than 8
 	if (strlen(directory) > 8) {
 		// printf("File length > 8");
 		return -ENAMETOOLONG;
 	}


	//I need to check if there is already a root directory in the .disk
	//Then if there is not one, I should write one to the .disk file
 	FILE *file = fopen(".disk", "r+b");
 	//If file = null, error has occurred
 	if (file == NULL) {
 		// printf("Error on fopen\n");
 		fclose(file);
 		return -1;
 	}

 	cs1550_root_directory root;
 	//Check if there is already a root in .disk
 	// printf("Read\n");
 	int readVal = fread(&root, sizeof(cs1550_root_directory), 1, file);
 	fclose(file);

 	if (readVal <= 0) {
 		// printf("Root error\n");
 		//fclose(file);
 		return returnVal;
 	}
 	if (root.nDirectories >= MAX_DIRS_IN_ROOT) {
 		// printf("Too many directories in root\n");
 		//fclose(file);
 		return -EPERM;
 	}


 	// printf("Not root\n");

 	int findDirReturn = findDirectory(directory);
 	if (findDirReturn != -1) {
 		// printf("Directory already exists\n");
 		// printf("returns - %d\n", findDirReturn);
 		return -EEXIST;
 	}


 	int directoryIndex = setBitmap(directory, '1');
 	if (directoryIndex != -1)
 		createDirectory(directory, directoryIndex);

 	free(directory);
 	free(filename);
 	free(extension);

 	return returnVal;
 }

/* 
 * Removes a directory.
 */
 static int cs1550_rmdir(const char *path)
 {
 	(void) path;
 	return 0;
 }

//mknod
/* 
 * Does the actual creation of a file. Mode and dev can be ignored.
 *
 */
 //For testing use - echo "" > file.txt
 static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
 {
 	// printf("MKNOD\n");
 	(void) mode;
 	(void) dev;
 	(void) path;

 	int returnVal = 0;

 	//sscanf
 	char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));

 	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 	//Check for errors
 	//File > 8 || extension > 3
 	if (strlen(filename) > 8 || strlen(extension) > 3) 
 		returnVal = -ENAMETOOLONG;

 	//File is trying to be created in root
 	if (strlen(filename) == 0)
 		returnVal = -EPERM;



 	cs1550_root_directory *root = calloc(1, sizeof(*root));
 	FILE *file = fopen(".disk", "r+b");
 	//Read the .disk file to get the root
 	fread(root, sizeof(*root), 1, file);
 	//Get the index of the directory in the root struct array
 	int findDirReturn = findDirectory(directory);
 	long nStartBlock = root->directories[findDirReturn].nStartBlock;

 	//Seek to the location of the directory in the file
 	fseek(file, sizeof(*root) * nStartBlock, SEEK_SET);
 	cs1550_directory_entry *dir = calloc(1, sizeof(*dir));
 	//Read the directory from the .disk file
 	fread(dir, sizeof(*dir), 1, file);

 	//File already exists
 	if (findFile(filename, extension, findDirReturn) != -1) {
 		// printf("mknod file exists\n");
 		returnVal = -EEXIST;
 	}


 	//Update the fields of directory struct
 	if (dir->nFiles < MAX_FILES_IN_DIR) {
 		//Concatenate the extension onto the filename
 		char fileAndExt[MAX_FILES_IN_DIR + MAX_EXTENSION + 5];
 		strcpy(fileAndExt, filename);
 		strcat(fileAndExt, ".");
 		strcat(fileAndExt, extension);
 		int fileStart = setBitmap(fileAndExt, '1');

 		//increment the number of files
 		dir->nFiles += 1;
 		//Copy the filename and extension
 		strcpy(dir->files[dir->nFiles - 1].fname, filename);
 		strcpy(dir->files[dir->nFiles - 1].fext, extension);
 		//Set the file size to 0
 		dir->files[dir->nFiles - 1].fsize = 0;
 		//Set the startblock of the file
 		dir->files[dir->nFiles - 1].nStartBlock = fileStart;


 		//Write the directory entry back to the file
 		fseek(file, sizeof(*dir) * nStartBlock, SEEK_SET);
 		fwrite(dir, sizeof(*dir), 1, file);


 		//Find a free block in the bitmap for the new inode
 		cs1550_inode *inode = calloc(1, sizeof(*inode));
	 	//Magic number stuff
 		inode->magic_number = 0xFFFFFFFF;
	 	//Set the number of children to be 0
 		inode->children = 0;
 		// printf("\nmknod inode seek to %ld\n", sizeof(*inode) * fileStart);
 		fseek(file, sizeof(*inode) * fileStart, SEEK_SET);
 		fwrite(inode, sizeof(*inode), 1, file);

 		fclose(file);

 		free(inode);
 	}
 	else {
 		fclose(file);
 		return returnVal;
 	}
 	
 	
 	free(root);
 	free(dir);
 	return returnVal;
 }

//Unlink
/*
 * Deletes a file
 */
 static int cs1550_unlink(const char *path)
 {
 	(void) path;
 	int returnVal = 0;
 	//sscanf
 	char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));

 	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 	if (strlen(filename) == 0)
 		returnVal = -EISDIR;

 	FILE *file = fopen(".disk", "r+b");

 	int findDirReturn = findDirectory(directory);

 	cs1550_root_directory *root = calloc(1, sizeof(*root));
 	fread(root, sizeof(*root), 1, file);
 	long dirStartBlock = root->directories[findDirReturn].nStartBlock;
 	

 	//Read in the directory from the .disk file
 	cs1550_directory_entry *dir = calloc(1, sizeof(*dir));
 	
 	fseek(file, sizeof(*dir) * dirStartBlock, SEEK_SET);
 	fread(dir, sizeof(*dir), 1, file);

 	int fileIndex = getFileIndex(filename, extension, dir);
 	if (fileIndex == -1)
 		return -ENOENT;
 	//get the starting block for the inode
 	long fileStartBlock = dir->files[fileIndex].nStartBlock;

 	//Read in the inode from the .disk file
 	cs1550_inode *inode = calloc(1, sizeof(*inode));
 	fseek(file, sizeof(*inode) * fileStartBlock, SEEK_SET);
 	fread(inode, sizeof(*inode), 1, file);

 	//Remove from bitmap
 	fseek(file, -BITMAP_SIZE, SEEK_END);
 	char *bitmap;
 	bitmap = malloc(sizeof(bitmap) * BITMAP_SIZE);
 	fread(bitmap, sizeof(char), BITMAP_SIZE, file);
 	int i;
 	//Iterate through the inode pointers array and free all of them from the bitmap

 	for (i = 0; i < inode->children; i++) {
 		// printf("bitmap[%ld] = %c\n", inode->pointers[i], bitmap[inode->pointers[i]]);
 		bitmap[inode->pointers[i]] = '0';
 		// printf("After - bitmap[%ld] = %c\n", inode->pointers[i], bitmap[inode->pointers[i]]);
 	}

 	//Free the inode from the bitmap
 	// printf("bitmap[fileStartBlock = %ld] = %c\n", fileStartBlock, bitmap[fileStartBlock]);
 	bitmap[fileStartBlock] = '0';
 	// printf("After - bitmap[fileStartBlock = %ld] = %c\n", fileStartBlock, bitmap[fileStartBlock]);

 	//Iterate through the inode pointers array to find the index with teh same start block
 	for (i = 0; i < dir->nFiles; i++) {
 		//Remove file from directory struct array
 		if (dir->files[i].nStartBlock == fileStartBlock) {
 			int j;
 			for (j = i; j < dir->nFiles - 1; j++) 
 				dir->files[j] = dir->files[j + 1];
 			break;
 		}
 	}

 	dir->nFiles -= 1;
 	fseek(file, sizeof(*dir) * dirStartBlock, SEEK_SET);
 	fwrite(dir, sizeof(*dir), 1, file);

 	fseek(file, -BITMAP_SIZE, SEEK_END);
 	fwrite(bitmap, sizeof(char), BITMAP_SIZE, file);

 	free(bitmap);
 	free(dir);
 	free(inode);
 	free(root);

 	fclose(file);

 	free(directory);
 	free(filename);
 	free(extension);
 	return 0;
 }

//Read
/* 
 * Read size bytes from file into buf starting from offset
 *
 */
 static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
 	struct fuse_file_info *fi)
 {
 	// printf("READ\n");
 	(void) buf;
 	(void) offset;
 	(void) fi;
 	(void) path;

 	int returnVal = -1;

 	//sscanf
 	char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));

 	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 	if (offset <= size)
 		returnVal = -EISDIR;


 	FILE *file = fopen(".disk", "r+b");

 	int findDirReturn = findDirectory(directory);

 	cs1550_root_directory *root = calloc(1, sizeof(*root));
 	fread(root, sizeof(*root), 1, file);
 	long dirStartBlock = root->directories[findDirReturn].nStartBlock;
 	

 	//Read in the directory from the .disk file
 	cs1550_directory_entry *dir = calloc(1, sizeof(*dir));
 	
 	fseek(file, sizeof(*dir) * dirStartBlock, SEEK_SET);
 	fread(dir, sizeof(*dir), 1, file);

 	int fileIndex = getFileIndex(filename, extension, dir);
 	//get the starting block for the inode
 	long fileStartBlock = dir->files[fileIndex].nStartBlock;

 	//Read in the inode from the .disk file
 	cs1550_inode *inode = calloc(1, sizeof(*inode));
 	fseek(file, sizeof(*inode) * fileStartBlock, SEEK_SET);
 	fread(inode, sizeof(*inode), 1, file);

 	// printf("Number of children: %d\n", inode->children);

 	int i;
 	for (i = 0; i < inode->children; i++) {
 		cs1550_disk_block *diskBlock = calloc(1, sizeof(*diskBlock));
 		//Find where the disk block starts
 		unsigned long diskStartBlock = inode->pointers[i];

 		//Read in the disk block
 		fseek(file, sizeof(*inode) * diskStartBlock, SEEK_SET);
 		fread(diskBlock, sizeof(*diskBlock), 1, file);
 		// printf("strlen(diskBlock->data = %d\n", strlen(diskBlock->data));

 		int j;
 		for (j = 0; j < strlen(diskBlock->data); j++) {
 			// printf("data[%d]: %c\n", j + strlen(buf), diskBlock->data[j + (offset % MAX_DATA_IN_BLOCK)]);
 			buf[j] = diskBlock->data[j + (offset % MAX_DATA_IN_BLOCK)];
 		}

 		// for (j = 0; j < strlen(buf); j++)
 		// 	printf("buf[%d]: %c\n", j, buf[j + strlen(buf)]);

 		free(diskBlock);
 	}


	//check to make sure path exists
	//check that size is > 0
	//check that offset is <= to the file size
	//read in data
	//set size and return, or error
 	fclose(file);

 	free(dir);
 	free(inode);
 	free(directory);
 	free(filename);
 	free(extension);

 	return size;
 }

//write
/* 
 * Write size bytes from buf into file starting from offset
 *
 */
 //test using echo "string" > test.txt or >> to append
 static int cs1550_write(const char *path, const char *buf, size_t size, 
 	off_t offset, struct fuse_file_info *fi)
 {
 	// printf("WRITE\n");

 	(void) buf;
 	(void) offset;
 	(void) fi;
 	(void) path;

 	//sscanf
 	char *directory = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *filename = calloc(1, sizeof(char) * (MAX_FILENAME + 1));
 	char *extension = calloc(1, sizeof(char) * (MAX_EXTENSION + 1));

 	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

 	//Find the index of the directory and the i
 	int findDirReturn = findDirectory(directory);
 	
 	FILE *file = fopen(".disk", "r+b");

 	cs1550_root_directory *root = calloc(1, sizeof(*root));
 	fread(root, sizeof(*root), 1, file);
 	long dirStartBlock = root->directories[findDirReturn].nStartBlock;
 	free(root);

 	//Read in the directory
 	cs1550_directory_entry *dir = calloc(1, sizeof(*dir));
 	fseek(file, sizeof(*dir) * dirStartBlock, SEEK_SET);
 	fread(dir, sizeof(*dir), 1, file);

 	//Get the index of the file in the dir->files array
 	int fileIndex = getFileIndex(filename, extension, dir);
 	//Get the start block of the file
 	long fileStartBlock = dir->files[fileIndex].nStartBlock;

 	//Check if the offset > size
 	if (offset > dir->files[fileIndex].fsize)
 		return -EFBIG;

 	//Seek to the file on .disk and read it into an inode struct
 	cs1550_inode *inode = calloc(1, sizeof(*inode));
 	// printf("Write inode seek to %ld\n", sizeof(*inode) * fileStartBlock);
 	fseek(file, sizeof(*inode) * fileStartBlock, SEEK_SET);
 	fread(inode, sizeof(*inode), 1, file);
 	// printf("inode->children = %d\n", inode->children);

 	if (inode->children == 0)
 		inode->children = 1;


 	cs1550_disk_block *diskBlock = calloc(1, sizeof(*diskBlock));
 	int positionInBlock = offset % MAX_DATA_IN_BLOCK;

 	// printf("Size = %ld\n", size);
 	// printf("positionInBlock = %d\n", positionInBlock);
 	
 	//If the position that we want to write to is not equal to 0
 	//Then we should read in the block from .disk and start to write to it
 	if (positionInBlock != 0 || size >= MAX_DATA_IN_BLOCK) {
 		// printf("APPEND\n");
 		// printf("diskStartBlock = %ld\n", inode->pointers[inode->children - 1]);
 		unsigned long diskStartBlock = inode->pointers[inode->children - 1];
	 	//Seek to the location of the disk block
 		fseek(file, sizeof(*inode) * diskStartBlock, SEEK_SET);

 		// printf("fread disk_block\n");
	 	//Read in the disk block
 		fread(diskBlock, sizeof(*diskBlock), 1, file);

 		long sizeToWrite = size;
	 	//Variable to keep track of the total amount of data we've written to the file
 		int totalDataWritten = 0;

	 	//While the max data we can store in a block - the offset - the amount of data we want to write is less than 0
		 	//I do offset % MAX_DATA because that calculation will give me the offset in the disk block that I need to write to
		 	//Where as the offset that is inputted is the offset from the start of the file -> ie first data block
 		// printf("Before while loop\n");
 		while ( (off_t)(MAX_DATA_IN_BLOCK - (offset % MAX_DATA_IN_BLOCK) - sizeToWrite) < 0) {
	 		//Write the maximum amount of data we can in the current block
 			int i;
 			// printf("Before for loop to copy into disk_block\n");
 			for (i = 0; i < MAX_DATA_IN_BLOCK - (offset % MAX_DATA_IN_BLOCK); i++) {
 				if (offset == 0)
 					diskBlock->data[(offset % MAX_DATA_IN_BLOCK) + i] = buf[i + totalDataWritten];
 				else
	 				diskBlock->data[(offset % MAX_DATA_IN_BLOCK) - 1 + i] = buf[i + totalDataWritten];	//- 1 in order to overwrite the \0 at the end

	 		}

	 		//Set the total amount of data we just wrote to be i (MAX_DATA - offset)
	 		totalDataWritten = i;

	 		printf("fwrite disk_block\n");
	 		fseek(file, sizeof(*diskBlock) * diskStartBlock, SEEK_SET);
	 		fwrite(diskBlock, sizeof(*diskBlock), 1, file);
	 		free(diskBlock);

	 		printf("calloc disk_block\n");
	 		//Make a new block
	 		diskBlock = calloc(1, sizeof(*diskBlock));
	 		int blockLocation = setBitmap("", '1');
	 		inode->children += 1;
	 		printf("Set pointer array\n");
	 		inode->pointers[inode->children - 1] = blockLocation;
	 		diskBlock->magic_number = 0xF113DA7A;

	 		//Reset the amount of data we need to write to the block
	 		sizeToWrite -= (sizeToWrite - (offset % MAX_DATA_IN_BLOCK) );
	 		//Reset the offset to 0
	 		offset = 0;
	 	}

	 	//Write the rest of the data to disk
	 	int i;
	 	for (i = 0; i < sizeToWrite; i++) {
	 		if (offset == 0)
	 			diskBlock->data[(offset % MAX_DATA_IN_BLOCK) + i] = buf[i + totalDataWritten];
	 		else
	 			diskBlock->data[(offset % MAX_DATA_IN_BLOCK) - 1 + i] = buf[i + totalDataWritten];	//- 1 in order to overwrite the \0 at the end
	 	}
	 	

	 	int blockLocation = inode->pointers[inode->children - 1];
	 	fseek(file, sizeof(*diskBlock) * blockLocation, SEEK_SET);
	 	fwrite(diskBlock, sizeof(*diskBlock), 1, file);
	 	free(diskBlock);
	 }
 	//If we don't have any children or the position that we want to write to in the block is 0
 	//and the number of children is less than the total number of pointers we are allowed to have
	 else if ( (inode->children == 0 || positionInBlock == 0) && (inode->children < NUM_POINTERS_IN_INODE) ) {
 		//Find a place for the new disk block in the bitmap
	 	int blockLocation = setBitmap("", '1');
	 	inode->pointers[inode->children - 1] = blockLocation;

	 	diskBlock->magic_number = 0xF113DA7A;

	 	int i;
	 	for (i = 0; i < size; i++) {
 			// printf("data[%d]: %c\n", i, diskBlock->data[i]);
 			// printf("buf[%d]: %c\n", i, buf[i]);
	 		diskBlock->data[i] = buf[i];
	 	}


	 	fseek(file, sizeof(*diskBlock) * blockLocation, SEEK_SET);
	 	fwrite(diskBlock, sizeof(*diskBlock), 1, file);
	 	free(diskBlock);
	 }

 	//Write the inode to disk
	 // printf("Inode seek to %ld\n", sizeof(*inode) * fileStartBlock);
	 fseek(file, sizeof(*inode) * fileStartBlock, SEEK_SET);
	 // printf("Inode num of children: %d\n", inode->children);
	 fwrite(inode, sizeof(*inode), 1, file);

 	//Misurda comments...
	//check to make sure path exists
	//check that size is > 0
	//check that offset is <= to the file size
	//write data
	//set size (should be same as input) and return, or error

	//Set the size of the file
	 dir->files[fileIndex].fsize = offset + size;

 	//Write the directory struct back to .disk
	 fseek(file, sizeof(*dir) * dirStartBlock, SEEK_SET);
	 fwrite(dir, sizeof(*dir), 1, file);
	 free(dir);
	 free(inode);

	 free(directory);
	 free(filename);
	 free(extension);


	 fclose(file);

	 return size;
	}

/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or 
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
 static int cs1550_truncate(const char *path, off_t size)
 {
 	(void) path;
 	(void) size;

 	return 0;
 }


/* 
 * Called when we open a file
 *
 */
 static int cs1550_open(const char *path, struct fuse_file_info *fi)
 {
 	(void) path;
 	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but 
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file 
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
 static int cs1550_flush (const char *path , struct fuse_file_info *fi)
 {
 	(void) path;
 	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
	.getattr	= cs1550_getattr,
	.readdir	= cs1550_readdir,
	.mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
	.read	= cs1550_read,
	.write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}

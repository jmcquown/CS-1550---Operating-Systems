#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "iso_font.h"
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>


typedef unsigned short color_t;

char *mapBuffer;
int openValue;

//Create the two structs for the screen information
struct fb_var_screeninfo varInfo;
struct fb_fix_screeninfo fixedInfo;

size_t screensize;


void init_graphics() {
	//Open the file from "dev/fb0"
	openValue = open("/dev/fb0", O_RDWR);
	// if (openValue == -1) {
	// 	printf("open() error\n");
	// 	exit(0);
	// }

	//Get the varibale screen information
	if (ioctl(openValue, FBIOGET_VSCREENINFO, &varInfo) == -1) {
		// printf("ioctl() varInfo error\n");
		exit(0);
	}

	//Get the fixed screen information
	if (ioctl(openValue, FBIOGET_FSCREENINFO, &fixedInfo) == -1) {
		// printf("ioctl() fixedInfo error\n");
		exit(0);
	}

	//Calculate the screen size by multiplying the number of horizontal lines by the length of each line in bytes
	screensize = varInfo.yres_virtual * fixedInfo.line_length;

	//Map the opened file into memory
	mapBuffer = (char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, openValue, 0);
	if ((int) mapBuffer == -1) {
		// printf("mmap() failure\n");
		exit(0);
	}
	// printf("%dx%d, %dbpp\n", varInfo.xres, varInfo.yres, varInfo.bits_per_pixel);

	//Need to disable keypress using ioctl
	struct termios terminal;
	ioctl(0, TCGETS, &terminal);
	terminal.c_lflag &= ~ICANON;
	terminal.c_lflag &= ~ECHO;
	ioctl(0, TCSETS, &terminal);
}

//Unmaps the file and closes it
void exit_graphics() {
	//Need to use ioctl to enable keypresses
	struct termios terminal;
	ioctl(0, TCGETS, &terminal);
	terminal.c_lflag |= ICANON;
	terminal.c_lflag |= ECHO;
	ioctl(0, TCSETS, &terminal);

	munmap(mapBuffer, screensize);
	close(openValue);
}

//Clear the screen using the ANSI escape code that was given in the project desciption
void clear_screen() {
	write(1, "\033[2J", 8);
}


char getkey() {
	//I got this stuff from the man pages.....
	fd_set rfds;
	struct timeval timeStruct;
	int returnVal;
	char c = '\0';

	timeStruct.tv_sec = 0;
	timeStruct.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);

	returnVal = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &timeStruct);
	if (returnVal > 0)
		read(0, &c, 1);
	return c;
}

void sleep_ms(long ms) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ms * 1000000;
	nanosleep(&ts, NULL);
}

void draw_pixel(int x, int y, color_t c) {
	
	int loc = x * (varInfo.bits_per_pixel / 8) + y * fixedInfo.line_length;
	//Check if the location of the pixel is out of bounds and will cause a seg fault
	if (loc < fixedInfo.line_length * varInfo.yres && loc >= 0)
		*(color_t *)(mapBuffer + loc) = c;
}

//In draw_rect I use 4 for loops in order to draw straight lines at the appropriate x and y coordinates
void draw_rect(int x1, int y1, int width, int height, color_t c) {
	int i;
	int x = x1;
	int y = y1;

	for (i = x1; i < (width + x); i++) {
		draw_pixel(x1, y1, c);
		x1 = x1 + 1;	//For some reason this worked as opposed to x1++
	}	
	for (i = y1; i < (height + y); i++) {
		draw_pixel(x1, y1, c);
		y1 = y1 + 1;
	}
	
	//Reset the values of x1 and y1 to the original starting coordinates
	x1 = x;
	y1 = y;
	for (i = y1; i < (height + y); i++) {
		draw_pixel(x1, y1, c);
		y1 = y1 + 1;
	}
	for (i = x1; i < (x + width); i++) {
		draw_pixel(x1, y1, c);
		x1 = x1 + 1;
	}
}

void fill_rect(int x1, int y1, int width, int height, color_t c) {
	int i;
	int j;
	int x = x1;
	int y = y1;
	for (x = x1; x < (width + x1); x++) {
		for (y = y1; y < (height + y1); y++) {
			draw_pixel(x, y, c);
		}
	}
}

void draw_char(int x, int y, int ascii, color_t color) {
	int i, j;
	//First for loop is to iterate through the iso_font array indices
	for (i = 0; i < 16; i++) {
		int value[8];
		//This for loop with iterate through each of the 8 bits in the iso_font index
		for (j = 0; j < 8; j++) {
			value[j] = (iso_font[ascii * 16 + i] & ( 1 << j )) >> j;
			//If the value is equal to 1, then draw the pixel
			if (value[j])
				draw_pixel(y + j, x + i, color);
		}
	}
}

void draw_text(int x, int y, const char *text, color_t c) {
	int i, offset = 0;
	// printf("%c\n", text);
	for (i = 0; text[i] != '\0'; i++) {
		draw_char(x, y + offset, text[i], c);
		offset = offset + 8;
	}
		

}



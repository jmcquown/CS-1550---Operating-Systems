#include <stdio.h>
#include <stdlib.h>

#define OFFSET 20

int main()
{
	int i;	
	int byte_number;	
	char write_value;	
	char in_value;
	FILE *disk;

	write_value = '1';
	byte_number = 0;

/*	disk = fopen("disk", "r+b");
	fseek(disk, -OFFSET, SEEK_END);	
	
	for (i = 0; i < 20; i++)
		fwrite(&write_value, sizeof write_value, 1, disk);
	
	fclose(disk); */

	disk = fopen(".disk", "r+b");

	while(fread(&in_value, sizeof in_value, 1, disk) != 0) {
		if (in_value != 0) {	
			printf("Value read: %c at %d\n", in_value, byte_number);
		}
		byte_number++;	
	}
	fclose(disk);

	return 0;
}



typedef unsigned short color_t;

void clear_screen();
void exit_graphics();
void init_graphics();
char getkey();
void sleep_ms(long ms);

void draw_pixel(int x, int y, color_t color);
void draw_rect(int x1, int y1, int width, int height, color_t c);
void fill_rect(int x1, int y1, int width, int height, color_t c);
void draw_text(int x, int y, const char *text, color_t c);
void draw_char(int x, int y, int ascii, color_t color);

int main(int argc, char** argv)
{
	int i;

	init_graphics();

	char key;
	int x = (640-20)/2;
	int y = (480-20)/2;
	clear_screen();
	
	draw_text(x, y, "Hello, World!", 0xFFFF);
	sleep_ms(10000);

	fill_rect(x + 100, y + 100, 200, 125, 0xFFFF);
	sleep_ms(10000);

	 do {
		draw_rect(x, y, 100, 60, 0);
		key = getkey();
		if(key == 'w') y-=10;
		else if(key == 's') y+=10;
		else if(key == 'a') x-=10;
		else if(key == 'd') x+=10;

		draw_rect(x, y, 100, 60, 0xFFFF);
		sleep_ms(10);
	}while (key != 'q');
	
	exit_graphics();

	return 0;

}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/keysymdef.h>

#define SIZE 30
#define GRID_X 10
#define GRID_Y 20

#define WIDTH 1200
#define HEIGHT 800
#define START_X WIDTH / 2
#define START_Y HEIGHT / 2

#define COLOR_TETRA_1 0xFF00FF
#define COLOR_TETRA_2 0xFF0000
#define COLOR_TETRA_3 0xFFFF00
#define COLOR_TETRA_4 0x00FF00
#define COLOR_TETRA_5 0xFF9700

#define FPS 60
#define FALL_SPEED 20

int FRAME_TIME = (int) (1000000 / FPS);
int frame_count = 0;

Display *display;
Window win;
GC gc;

unsigned long colori[] = {COLOR_TETRA_1, COLOR_TETRA_2, COLOR_TETRA_3, COLOR_TETRA_4, COLOR_TETRA_5};
char text[256];
static KeySym key_sym;
int start_grid_x = START_X - ((GRID_X * SIZE) / 2);
int start_grid_y = START_Y - ((GRID_Y * SIZE) / 2);
unsigned int count = 0;
unsigned int is_lose = 0;

typedef struct{
	int x, y;
	int index_x, index_y;
	unsigned long color;
	int color_id;
}Cube;

typedef struct{
	Cube shape[4];
	int pivot_x, pivot_y;
	unsigned long color;
	int type;
}Tetra;

int grid[GRID_X][GRID_Y];
Tetra *runner = NULL;

void send_tetra();
void rotate_tetra();

void init_grid();
void draw_grid(int x, int y, int sx, int sy);
void draw_grids_cube();
void update_grid();
void move_runner_lat(int off_x);
void printf_grid();

void init_x();  
void close_x(); 
void redraw();
void i_lose();

int main(){

	display = XOpenDisplay(NULL);	
	if(display == NULL) return 1;

	init_x();	
	init_grid();
	runner = malloc(sizeof(Tetra));
	if(runner == NULL) { perror("Malloc Error"); return 1;}
	send_tetra(rand() % 5);
	
	printf("\x1b[9C|Tetramino|");
	int size_px = GRID_X * SIZE;
	int size_py = GRID_Y * SIZE;
	printf("\x1b[2J\x1b[H");
	while(1)
	{
            	frame_count = (frame_count+1)%FALL_SPEED;
		XEvent event;
		while (XPending(display) > 0) {
        		XNextEvent(display, &event);
		
			if(event.type == KeyPress){
				key_sym = XLookupKeysym(&event.xkey, 0);
			        if (key_sym == XK_Escape) {
                                        close_x();
                                        exit(1);
                                }else if (key_sym == XK_Right) {
					move_runner_lat(1);
				}else if(key_sym == XK_Left){
					move_runner_lat(-1);
				}else if(key_sym == XK_Up){
					rotate_tetra();
				}else if(key_sym == XK_Down){
                                        update_grid();
                                }
                                int l = XLookupString(&event.xkey, text, sizeof(text) - 1, &key_sym, 0);
                                if(l > 0){
                                        text[l] = '\0';
                                        //printf("You pressed the key: %c\n", text[0]);
                                }

			}
    		}
		if(is_lose) i_lose();
		if(frame_count == 0) update_grid();
                XClearWindow(display, win);                     
                draw_grids_cube();                              
                draw_grid(start_grid_x, start_grid_y, size_px, size_py);
                printf_grid();

		usleep(FRAME_TIME);
	}
	free(runner);
}

void i_lose(){

	close_x();
	exit(1);
}

void init_grid(){
	for(int i = 0; i < GRID_X; i++)
                for(int j = 0; j < GRID_Y; j++){
                        grid[i][j] = -1;
        }

}

int check_if_lose(int a[8]){
	if(grid[a[0]][a[1]] >= 0) { is_lose = 1; return 1; }
	if(grid[a[2]][a[3]] >= 0) { is_lose = 1; return 1; }
	if(grid[a[4]][a[5]] >= 0) { is_lose = 1; return 1; }
	if(grid[a[6]][a[7]] >= 0) { is_lose = 1; return 1; }

	return 0;
}

void create_tetra(int a[8], int pivot_x, int pivot_y, int color){
	XSetForeground(display, gc, colori[color]);
        int i = 0, j = 0;
	runner->color = colori[color];
	runner->type = color;
	runner->pivot_x = pivot_x;
	runner->pivot_y = pivot_y;
	while(i < 4){
		Cube *c = &runner->shape[i];
		c->index_x = a[j];
		c->index_y = a[j+1];
		c->x = start_grid_x + (SIZE * a[j]);
		c->y = start_grid_y + (SIZE * a[j+1]);
		c->color = colori[color];
		c->color_id = runner->type;
		grid[a[j]][a[j+1]] = color;
		i++; j += 2;
	}
	count++; 
}

void rotate_tetra(){
	int new_x[4];
	int new_y[4];
	int can_rotate = 1;
	Tetra *r = runner;
	for(int i = 0; i<4; i++){
		int nx = r->pivot_x - (r->shape[i].index_y - r->pivot_y);
		int ny = r->pivot_y + (r->shape[i].index_x - r->pivot_x); 
		new_x[i] = nx;
		new_y[i] = ny;
		//check
		if (nx < 0 || nx >= GRID_X || ny < 0 || ny >= GRID_Y) {
			can_rotate = 0;
			break;
		}
		if(grid[nx][ny] >= 0){
			int is_self = 0;
			for (int j = 0; j < 4; j++) {
				if (runner->shape[j].index_x == nx && runner->shape[j].index_y == ny) {
					is_self = 1;
					break;
				}
			}
			if(is_self == 0){ can_rotate = 0; break; }
		}
	}

	if(can_rotate && runner->type != 2){
	int color_tmp;
		for(int i = 0; i< 4; i++){
			Cube *c = &runner->shape[i];
			color_tmp = grid[c->index_x][c->index_y];
			grid[c->index_x][c->index_y] = -1;
		}
		for(int i = 0; i < 4; i++)
		{
			Cube *c = &runner->shape[i];
			c->index_x = new_x[i]; 
			c->index_y = new_y[i];
			c->x = start_grid_x + (SIZE * c->index_x);
			c->y = start_grid_y + (SIZE * c->index_y);
			c->color = color_tmp;
			grid[c->index_x][c->index_y] = runner->type;
		}
	}	

}

void send_tetra(int type){
	int a[8];
	if(type == 0){
		int tmp[] = {4,0,5,0,6,0,5,1};
		memcpy(a, tmp, sizeof a);
		if(check_if_lose(a) == 0) create_tetra(a, 5, 0, type);
		//printf("Type is: %d\n", type);
	}else if(type == 1){
		int tmp[] = {3,0,4,0,5,0,6,0};
		memcpy(a, tmp, sizeof a);
                if(check_if_lose(a) == 0) create_tetra(a, 4, 0, type);
	}else if(type == 2){
		int tmp[] = {4,0,5,0,4,1,5,1};
                memcpy(a, tmp, sizeof a);
		if(check_if_lose(a) == 0) create_tetra(a, -1, -1, type);
        }else if(type == 3){
		int tmp[] = {4,0,4,1,4,2,5,2};
                memcpy(a, tmp, sizeof a);
		if(check_if_lose(a) == 0) create_tetra(a, 4, 1, type);
        }else if(type == 4){
		int tmp[] = {4,0,4,1,5,1,5,2}; 
                memcpy(a, tmp, sizeof a);
		if(check_if_lose(a) == 0) create_tetra(a, 4, 1, type);
        }
}

void draw_cube(Cube *c){
	XSetForeground(display, gc, c->color);
        XFillRectangle(display, win, gc, c->x, c->y, SIZE, SIZE);
}

void draw_cube_new(int i, int j){
        XSetForeground(display, gc, colori[grid[i][j]]);
        XFillRectangle(display, win, gc, start_grid_x + (SIZE * i), start_grid_y + (SIZE * j), SIZE, SIZE);
}

int check_tetra(){
	for(int i = 0; i < 4; i++) 
        {
		Cube *c = &runner->shape[i];
		if(c->index_y+1 >= GRID_Y) return 0;
		if(grid[c->index_x][c->index_y+1] >= 0){
			int is_self = 0;
			for(int j = 0; j < 4; j++){
				if(runner->shape[j].index_x == c->index_x && runner->shape[j].index_y == c->index_y + 1) {
                    			is_self = 1;
                    			break;
				}
			}
			if(is_self == 0) return 0;
		}
	}
	return 1;
}

int check_tetra_lat(int off_x){
        for(int i = 0; i < 4; i++)
        {
                Cube *c = &runner->shape[i];
                if(c->index_x+off_x >= GRID_X || c->index_x+off_x < 0) return 0;
                if(grid[c->index_x+off_x][c->index_y] >= 0){
                        int is_self = 0;
                        for(int j = 0; j < 4; j++){
                                if(runner->shape[j].index_x == c->index_x+off_x && runner->shape[j].index_y == c->index_y) {
                                        is_self = 1;
                                        break;
                                }
                        }
                        if(is_self == 0) return 0;
                }
        }
        return 1;
}

void move_runner_lat(int off_x){
	if(check_tetra_lat(off_x)){
                int color_tmp;
                for(int i = 0; i < 4; i++)
                {
                        Cube *c = &runner->shape[i];
                        color_tmp = grid[c->index_x][c->index_y];
                        grid[c->index_x][c->index_y] = -1;
                }
                for(int i = 0; i < 4; i++)
                {
                        Cube *c = &runner->shape[i];
                        c->index_x += off_x;
                        grid[c->index_x][c->index_y] = color_tmp;
                        c->y = start_grid_y + (SIZE * c->index_y);
                }
		runner->pivot_x+=off_x;
        }
}

void update_grid(){
	if(check_tetra()){
		int color_tmp;
		for(int i = 0; i < 4; i++) 
		{
			Cube *c = &runner->shape[i];
			color_tmp = grid[c->index_x][c->index_y];
			grid[c->index_x][c->index_y] = -1;
		}
		for(int i = 0; i < 4; i++) 
                {
			Cube *c = &runner->shape[i];
			c->index_y++;
			grid[c->index_x][c->index_y] = color_tmp;
			c->y = start_grid_y + (SIZE * c->index_y);
		}
		runner->pivot_y++;
	}else{
		send_tetra(rand() % 5);
	}
}

void draw_grid(int x, int y, int sx, int sy){
	XSetForeground(display, gc, 0xFFFFFF);
	for(int i = 0; i <= GRID_X; i++)
		XDrawLine(display, win, gc, x + (i * SIZE), y,     x + (i * SIZE), y + sy);
        for(int j = 0; j <= GRID_Y; j++)
		XDrawLine(display, win, gc, x, y + (j * SIZE),     x + sx, y + (j * SIZE));
	
}

void draw_grids_cube(){
	for(int i = 0; i < GRID_X; i++)
		for(int j = 0; j < GRID_Y; j++){
			if(grid[i][j] >= 0) draw_cube_new(i, j);
	}
}

void printf_grid(){
	
	printf("\x1B[%dA", GRID_Y +2);	
	for(int j = 0; j < GRID_Y; j++){
                for(int i = 0; i < GRID_X; i++){
			int c = grid[i][j];	
                        if(c >= 0) printf("\x1b[1;%d;%dm[x]\x1b[1;39;49m", 31+c, 41+c);
			else printf("[ ]");
        	}
		printf("\n");
	}
	printf("Point: %d\n", count);
	//fflush(stdout);
}

void init_x(){
        win = XCreateSimpleWindow(
                display,
                XDefaultRootWindow(display),
                0, 0,
                WIDTH, HEIGHT,
                0,
                0x00000000,
                0x00000000
        );

        gc = XCreateGC(display, win, 0, NULL);

        XStoreName(display, win, "First X11 window");
        XSelectInput(display, win,
                ExposureMask|ButtonPressMask|KeyPressMask|KeyReleaseMask);
        XMapWindow(display, win);
}

void close_x(){

	XFreeGC(display, gc);
        XDestroyWindow(display,win);	
    	XCloseDisplay(display);
}

/*

                  <TYPE>                                                <GRID>
                                                            0  1  2  3  4  5  6  7  8  9
                                                           [ ][ ][ ][ ][x][x][x][ ][ ][ ]  0
 1. [x][x][x]    2. [x][x][x][x]    3. [x][x]              [ ][ ][ ][ ][ ][x][ ][ ][ ][ ]  1
       [x]                             [x][x]              [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]  2
                                                           [ ][ ][ ][x][x][x][x][ ][ ][ ]  3 
 4. [x]          5. [x]                                    [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]  4
    [x]             [x][x]                                 [ ][ ][ ][ ][x][x][ ][ ][ ][ ]  5
    [x][x]             [x]                                 [ ][ ][ ][ ][x][x][ ][ ][ ][ ]  6 
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]  7
                                                           [ ][ ][ ][ ][x][ ][ ][ ][ ][ ]  8
                                                           [ ][ ][ ][ ][x][ ][ ][ ][ ][ ]  9
                                                           [ ][ ][ ][ ][x][x][ ][ ][ ][ ] 10
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 11
                                                           [ ][ ][ ][ ][x][ ][ ][ ][ ][ ] 12
                                                           [ ][ ][ ][ ][x][x][ ][ ][ ][ ] 13
                                                           [ ][ ][ ][ ][ ][x][ ][ ][ ][ ] 14
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 15
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 16
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 17
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 18
                                                           [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ] 19

*/
 

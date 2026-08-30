#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/keysymdef.h>

#define SIZE 30 
#define GRID_X 10
#define GRID_Y 20
#define TINY_GRID_X 6
#define TINY_GRID_Y 5

#define WIDTH  1200
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
const int start_grid_x = START_X - ((GRID_X * SIZE) / 2);
const int start_grid_y = START_Y - ((GRID_Y * SIZE) / 2);
int start_grid_next_x = start_grid_x + ((GRID_X + 1)*SIZE);
int start_grid_next_y = start_grid_y;

unsigned int count = 0;
unsigned int is_lose = 0;
unsigned int is_pause = 0;

typedef struct{
	int x, y;
	int index_x, index_y;
	unsigned long color;
	int color_id;
}Cube;

typedef struct Tetra{
	Cube shape[4];
	int pivot_x, pivot_y;
	unsigned long color;
	int type;
	struct Tetra *shadow;
}Tetra;

int grid[GRID_X][GRID_Y];
int grid_next[GRID_X][GRID_Y];
Tetra *runner = NULL;
Tetra *next_run = NULL;
int next_tetra;

void send_tetra(int type);
void send_next(int type);
void rotate_tetra();

void init_grid(int g[GRID_X][GRID_Y]);
void draw_grid(int x, int y, int sx, int sy, int num_x, int num_y);
void draw_grids_cube(int g[GRID_X][GRID_Y], int size_x, int size_y, int start_x, int start_y);
void update_grid();
void move_runner_lat(int off_x);
void printf_grid();

void draw_text(char *c, int x, int y);

void init_x();  
void close_x(); 
void i_lose();

int main(){
	srand(time(NULL));
	display = XOpenDisplay(NULL);	
	if(display == NULL) return 1;

	init_x();	
	init_grid(grid);
	init_grid(grid_next);
	//XWindowAttributes *attr;	

	runner = malloc(sizeof(Tetra));
	next_run = malloc(sizeof(Tetra));
	if(runner == NULL || next_run == NULL) { perror("Malloc Error"); return 1;}
	send_tetra(rand() % 5);
	next_tetra = (rand() % 5);
	send_next(next_tetra);
	
	printf("\x1b[9C|Tetramino|%d", next_tetra);
	int size_px = GRID_X * SIZE;
	int size_py = GRID_Y * SIZE;
	int size_px_next = TINY_GRID_X * SIZE;
	int size_py_next = TINY_GRID_Y * SIZE;
	printf("\x1b[2J\x1b[H");
	while(1)
	{
            	frame_count = (frame_count+1)%FALL_SPEED;
		XEvent event;
		/*XGetWindowAttributes(display, win, attr);
		WIDTH = attr->width;
		HEIGHT = attr->height;*/

		while (XPending(display) > 0) {
        		XNextEvent(display, &event);
		
			if(event.type == KeyPress){
				key_sym = XLookupKeysym(&event.xkey, 0);
			        if (key_sym == XK_Escape) {
                                        close_x();
                                        exit(1);
				}
				if(!is_pause){
                                	if (key_sym == XK_Right) {
						move_runner_lat(1);
					}else if(key_sym == XK_Left){
						move_runner_lat(-1);
					}else if(key_sym == XK_Up){
						rotate_tetra();
					}else if(key_sym == XK_Down){
                                        	update_grid();
                                	}
                                }
				int l = XLookupString(&event.xkey, text, sizeof(text) - 1, &key_sym, 0);
                                if(l > 0){
                                        text[l] = '\0';
					if(text[0] == 'p') is_pause = !is_pause;
                                }

			}
    		}
		if(is_lose) i_lose();
		if(frame_count == 0 && !is_pause){ update_grid(); send_next(next_tetra); }
                XClearWindow(display, win);                     

                draw_grids_cube(grid, GRID_X, GRID_Y, start_grid_x, start_grid_y);                              
                draw_grids_cube(grid_next, TINY_GRID_X, TINY_GRID_Y, start_grid_next_x, start_grid_next_y);
		draw_grid(start_grid_x, start_grid_y, size_px, size_py, GRID_X, GRID_Y);
		draw_grid(start_grid_next_x, start_grid_next_y, size_px_next, size_py_next, TINY_GRID_X, TINY_GRID_Y);
		//char c[] = "NEXT";
	//	draw_text(c, start_grid_next_x + (size_px_next/2), start_grid_next_y + size_px_next);
                printf_grid();


		usleep(FRAME_TIME);
	}
	free(runner);
	free(next_run);
}

/*void draw_text(char *c, int x, int y){
	XTextItem item = {c, strlen(c), 0, None};
	XDrawText(display, win, gc, x- strlen(c)/2, y, &item, 1);
}*/

void i_lose(){
	printf("YOU LOSE, AHAHAH\n");
	close_x();
	exit(1);
}

void init_grid(int g[GRID_X][GRID_Y]){
	for(int i = 0; i < GRID_X; i++)
                for(int j = 0; j < GRID_Y; j++){
                        g[i][j] = -1;
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
/*Send next scrivo il codice piu volte*/
void create_next(int a[8], int color){
	init_grid(grid_next);
        XSetForeground(display, gc, colori[color]);
        int i = 0, j = 0;
        next_run->color = colori[color];
        next_run->type = color;
        while(i < 4){
                Cube *c = &next_run->shape[i];
                c->index_x = a[j];
                c->index_y = a[j+1];
                c->x = start_grid_x + (SIZE * a[j]);
                c->y = start_grid_y + (SIZE * a[j+1]);
                c->color = colori[color];
                c->color_id = next_run->type;
                grid_next[a[j]][a[j+1]] = color;
                i++; j += 2;
        }
}

void send_next(int type){
        int a[8];
        if(type == 0){
                int tmp[] = {1,1,2,1,3,1,2,2};
                memcpy(a, tmp, sizeof a);
                create_next(a, type);
                //printf("Type is: %d\n", type);
        }else if(type == 1){
                int tmp[] = {1,2,2,2,3,2,4,2};
                memcpy(a, tmp, sizeof a);
                create_next(a, type);
        }else if(type == 2){
                int tmp[] = {2,1,3,1,2,2,3,2};
                memcpy(a, tmp, sizeof a);
                create_next(a, type);
        }else if(type == 3){
                int tmp[] = {2,1,2,2,2,3,3,3};
                memcpy(a, tmp, sizeof a);
                create_next(a, type);
        }else if(type == 4){
                int tmp[] = {2,1,2,2,3,2,3,3};
                memcpy(a, tmp, sizeof a);
                create_next(a, type);
        }
}

void draw_cube(Cube *c){
	XSetForeground(display, gc, c->color);
        XFillRectangle(display, win, gc, c->x, c->y, SIZE, SIZE);
}

void draw_cube_new(int g[GRID_X][GRID_Y], int i, int j, int sx, int sy){
        XSetForeground(display, gc, colori[g[i][j]]);
        XFillRectangle(display, win, gc, sx + (SIZE * i), sy + (SIZE * j), SIZE, SIZE);
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

void move_grid_down(int y_del){
	for (int y = y_del; y > 0; y--) {
        	for (int x = 0; x < GRID_X; x++) {
            		grid[x][y] = grid[x][y-1];
        	}
    	}
    	for (int x = 0; x < GRID_X; x++) {
        	grid[x][0] = -1;
    	}
}

void check_delete_update_rows(){
	for(int y = 0; y < GRID_Y; y++ ){
		int must_delete = 1;
		for(int x = 0; x < GRID_X; x++){
			if(grid[x][y] < 0) must_delete = 0;
		}
		if(must_delete){
			for(int x = 0; x < GRID_X; x++){
                        	grid[x][y] = -1;
                	}
			move_grid_down(y);
		}
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
		check_delete_update_rows();
		send_tetra(next_tetra);
		next_tetra = rand() % 5;
	}
}

void draw_grid(int x, int y, int sx, int sy, int nx_cube, int ny_cube){
	XSetForeground(display, gc, 0xFFFFFF);
	for(int i = 0; i <= nx_cube; i++)
		XDrawLine(display, win, gc, x + (i * SIZE), y,     x + (i * SIZE), y + sy);
        for(int j = 0; j <= ny_cube; j++)
		XDrawLine(display, win, gc, x, y + (j * SIZE),     x + sx, y + (j * SIZE));
	
}

void draw_grids_cube(int g[GRID_X][GRID_Y], int size_x, int size_y, int start_x, int start_y){
	for(int i = 0; i < size_x; i++)
		for(int j = 0; j < size_y; j++){
			if(g[i][j] >= 0) draw_cube_new(g, i, j, start_x, start_y);
	}
}

void printf_grid(){
	char point[128];
	char next_msg[128];
	char *is_full  = "  ";
	char *is_empty = "  ";
	int size_cell  = strlen(is_full);
	int tot_len    = GRID_X * size_cell;
	int tot_len_grid_next = TINY_GRID_X * size_cell;
	snprintf(point, sizeof(point), "Lvl: %d Next: %d", count, next_tetra);
	snprintf(next_msg, sizeof(point), "Next");
//	printf("\x1B[%dA", GRID_Y +2);
printf("\033[H");	
	printf("\u2554");
	for(int i = 0;  i < tot_len; i++) printf("\u2550");
	printf("\u2557\u2554");
	for(int i = 0;  i < tot_len_grid_next; i++) printf("\u2550");
        printf("\u2557\n");

	/* grid */
	for(int j = 0; j < GRID_Y; j++){
                printf("\u2551");
		for(int i = 0; i < GRID_X; i++){
			int c = grid[i][j];	
                        if(c >= 0) printf("\x1b[1;%d;%dm%s\x1b[1;39;49m", 31+c, 41+c, is_full);
			else printf("%s", is_empty);
        	}
		printf("\u2551");

		if(j < TINY_GRID_Y){
			//printf new_grid
			printf("\u2551");
			for(int i = 0; i < TINY_GRID_X; i++){
                        	int c = grid_next[i][j];     
                        	if(c >= 0) printf("\x1b[1;%d;%dm%s\x1b[1;39;49m", 31+c, 41+c, is_full);
                        	else printf("%s", is_empty);
                	}
			printf("\u2551\n");
		}else if(j == TINY_GRID_Y){
			// print end next
        		printf("\u255A\u2550 %s ", next_msg);
			int start_next = strlen(next_msg)+3;
        		for(int i = start_next; i < tot_len_grid_next; i++) printf("\u2550");
		        printf("\u255D\n");
		}else printf("\n"); 
	}
	printf("\u255A\u2550 %s ", point);
	int start_point = strlen(point) + 3;
	for(int i = start_point;  i < tot_len; i++) printf("\u2550");	
	printf("\u255D\n");

	fflush(stdout);
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

        XStoreName(display, win, "Tetramino");
        XSelectInput(display, win,
                ExposureMask|ButtonPressMask|KeyPressMask|KeyReleaseMask);
        XMapWindow(display, win);

}

void close_x(){

	XFreeGC(display, gc);
        XDestroyWindow(display,win);	
    	XCloseDisplay(display);
}


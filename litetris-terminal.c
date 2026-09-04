#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define SIZE 30 
#define GRID_X 10
#define GRID_Y 20
#define TINY_GRID_X 6
#define TINY_GRID_Y 5

#define WIDTH  1200
#define HEIGHT 800
#define START_X WIDTH / 2
#define START_Y HEIGHT / 2

#define FPS 60
#define FALL_SPEED 20

int FRAME_TIME = (int) (1000000 / FPS);
int frame_count = 0;

const int start_grid_x = START_X - ((GRID_X * SIZE) / 2);
const int start_grid_y = START_Y - ((GRID_Y * SIZE) / 2);

unsigned int count = 0;
unsigned int is_lose = 0;
unsigned int is_pause = 0;

typedef struct{
	int x, y;
	int index_x, index_y;
}Cube;

typedef struct Tetra{
	Cube shape[4];
	int pivot_x, pivot_y;
	int type;
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
void update_grid();
void move_runner_lat(int off_x);
void printf_grid();

void init_close();
void i_lose();

// terminos
struct termios restore_termios;

void disable_raw_mode(){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &restore_termios);
}

void enable_raw_mode(void){
	tcgetattr(STDIN_FILENO, &restore_termios);
	atexit(disable_raw_mode);
 
	struct termios raw = restore_termios;
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
 
	raw.c_cc[VMIN]  = 0;
	raw.c_cc[VTIME] = 0;
 
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void input_terminal_moves(){
    char c;
    int n = read(STDIN_FILENO, &c, 1);
    if(n <= 0) return;

    if(c == 27){
        char seq[2];
        if(read(STDIN_FILENO, &seq[0], 1) != 1 ||
           read(STDIN_FILENO, &seq[1], 1) != 1){
            disable_raw_mode();
            init_close();
        }
        if(seq[0] == '[' && !is_pause){
            switch(seq[1]){
                case 'C': move_runner_lat(1);  break;
                case 'D': move_runner_lat(-1); break;
                case 'A': rotate_tetra();      break;
                case 'B': update_grid();       break;
            }
        }
        return;
    }

    if(c == 'q'){
        disable_raw_mode();
        init_close();
    }

    if(c == 'p'){ is_pause = !is_pause; return; }

    if(!is_pause){
        if(c == 'd')      move_runner_lat(1);
        else if(c == 'a') move_runner_lat(-1);
        else if(c == 'w') rotate_tetra();
        else if(c == 's') update_grid();
    }
}

int main(){
	srand(time(NULL));
	enable_raw_mode();	
	init_grid(grid);
	init_grid(grid_next);

	runner = malloc(sizeof(Tetra));
	next_run = malloc(sizeof(Tetra));
	if(runner == NULL || next_run == NULL) { perror("Malloc Error"); return 1;}
	send_tetra(rand() % 5);
	next_tetra = (rand() % 5);
	send_next(next_tetra);
	
	printf("\x1b[2J\x1b[H");
	while(1)
	{
        frame_count = (frame_count+1)%FALL_SPEED;
		input_terminal_moves();

		if(is_lose) i_lose();
		if(frame_count == 0 && !is_pause){ update_grid(); send_next(next_tetra); }
                printf_grid();
		usleep(FRAME_TIME);
	}
}

void i_lose(){
	printf("YOU LOSE!\n");
	init_close();
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
        int i = 0, j = 0;
	runner->type = color;
	runner->pivot_x = pivot_x;
	runner->pivot_y = pivot_y;
	while(i < 4){
		Cube *c = &runner->shape[i];
		c->index_x = a[j];
		c->index_y = a[j+1];
		c->x = start_grid_x + (SIZE * a[j]);
		c->y = start_grid_y + (SIZE * a[j+1]);
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
		for(int i = 0; i< 4; i++){
			Cube *c = &runner->shape[i];
			grid[c->index_x][c->index_y] = -1;
		}
		for(int i = 0; i < 4; i++)
		{
			Cube *c = &runner->shape[i];
			c->index_x = new_x[i]; 
			c->index_y = new_y[i];
			c->x = start_grid_x + (SIZE * c->index_x);
			c->y = start_grid_y + (SIZE * c->index_y);
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
        int i = 0, j = 0;
        next_run->type = color;
        while(i < 4){
                Cube *c = &next_run->shape[i];
                c->index_x = a[j];
                c->index_y = a[j+1];
                c->x = start_grid_x + (SIZE * a[j]);
                c->y = start_grid_y + (SIZE * a[j+1]);
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

int check_tetra_move(int off_x, int off_y){
	for(int i = 0; i < 4; i++) {
        Cube *c = &runner->shape[i];
        int target_x = c->index_x + off_x;
        int target_y = c->index_y + off_y;

        if(target_x < 0 || target_x >= GRID_X || target_y >= GRID_Y || target_y < 0) {
            return 0;
        }

        if(grid[target_x][target_y] >= 0) {
            int is_self = 0;
            for(int j = 0; j < 4; j++) {
                if(runner->shape[j].index_x == target_x && runner->shape[j].index_y == target_y) {
                    is_self = 1;
                    break;
                }
            }
            if(!is_self) return 0;
        }
    }
    return 1;
}

int check_tetra() {
    return check_tetra_move(0, 1);
}

int check_tetra_lat(int off_x) {
    return check_tetra_move(off_x, 0);
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

void printf_grid(){
	char title[128];
	char point[128];
	char next_msg[128];
	char *is_full  = "  ";
	char *is_empty = " .";
	int size_cell  = strlen(is_full);
	int tot_len    = GRID_X * size_cell;
	int tot_len_grid_next = TINY_GRID_X * size_cell;
	snprintf(title, sizeof(title), "LITETRIS");
	snprintf(point, sizeof(point), "Lvl: %d", count);
	snprintf(next_msg, sizeof(next_msg), "Next");

	//upper title
	printf("\033[H");	
	printf("\u2554\u2550\u2550\u2550\u2550\u2550 %s ", title);
	int start_next = strlen(title)+7;
	for(int i = start_next; i < tot_len; i++) printf("\u2550");
	//upper next	
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
			printf("\u2551");
			for(int i = 0; i < TINY_GRID_X; i++){
                        	int c = grid_next[i][j];     
                        	if(c >= 0) printf("\x1b[1;%d;%dm%s\x1b[1;39;49m", 31+c, 41+c, is_full);
                        	else printf("%s", is_empty);
                	}
			printf("\u2551\n");
		}else if(j == TINY_GRID_Y){
			// print end next
        		printf("\u255A\u2550\u2550\u2550 %s ", next_msg);
			int start_next = strlen(next_msg)+5;
        		for(int i = start_next; i < tot_len_grid_next; i++) printf("\u2550");
		        printf("\u255D\n");
		}else printf("\n"); 
	}
	printf("\u255A\u2550\u2550\u2550\u2550\u2550 %s ", point);
	int start_point = strlen(point) + 7;
	for(int i = start_point;  i < tot_len; i++) printf("\u2550");	
	printf("\u255D\n");

	fflush(stdout);
}

void init_close(){
	free(runner);
	free(next_run);
	exit(1);
}

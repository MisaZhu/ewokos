#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <sys/keydef.h>

#define WIN_WIDTH		55
#define WIN_HEIGHT		50
#define SCALE_FACTOR	4
#define SNAKE_DEF_LEN	20
#define SNAKE_MAX_LEN	256
#define FOOD_LIFE		100 

#define SNAKE_COLOR		0xFFFFFFFF
#define FOOD_COLOR		0xFF00FF00
#define BG_COLOR		0xFF000000
#define TEXT_COLOR		0xAFFFFFFF

#define RANDOM(x, m) 	((x * seed + 2718) % m)

static int seed = 31415;

static bool top = false;
void on_focus(xwin_t* x) {
	(void)x;
	top = true;
}

void on_unfocus(xwin_t* x) {
	(void)x;
	top = false;
}

enum DIRECTION
{
  UP=1, 
  DOWN, 
  LEFT, 
  RIGHT,
};

typedef struct{
	int x;
	int y;
}dot;

typedef struct{
	int len;
	int score;
	dot body[SNAKE_MAX_LEN];	
}snake;

typedef struct{
	int life;
	dot where;
}food;

void snake_move(snake *s, int dir){
	int i;
	for(i = s->len - 1; i > 0; i--){
		s->body[i].x = s->body[i-1].x;
		s->body[i].y = s->body[i-1].y;
	}

	switch(dir){
		case UP:
			s->body[0].y--;
			s->body[0].y += WIN_HEIGHT;
			s->body[0].y %= WIN_HEIGHT;
		break;
		case DOWN:
            s->body[0].y++;
			s->body[0].y %= WIN_HEIGHT;
        break;
		case RIGHT:
            s->body[0].x++;
			s->body[0].x %= WIN_WIDTH;
        break;
		case LEFT:
            s->body[0].x--;
			s->body[0].x += WIN_WIDTH;
			s->body[0].x %= WIN_WIDTH;
        break;
		default:
		break;
	}
}

void draw_dot(graph_t *g, int x, int y, int scale, int color){
	int i,j;

	for(i = 0; i < SCALE_FACTOR; i++)
		for( j = 0; j < SCALE_FACTOR; j++)
			graph_pixel_safe(g, x * scale + i, 
						  y * scale + j, 
						  color);
}

void refresh_food(food *f){
	f->life = FOOD_LIFE;
	f->where.x = RANDOM(f->where.x , WIN_WIDTH);
	f->where.y = RANDOM(f->where.y , WIN_HEIGHT);
}

void snake_draw(graph_t *g, snake *s, food *f){

    int i;
    for(i=0; i<s->len; i++){
		draw_dot(g, s->body[i].x, s->body[i].y, SCALE_FACTOR, SNAKE_COLOR);
	} 

	if(f->life){
		f->life--;
		if(f->life > 10 || (f->life%2))
			draw_dot(g, f->where.x, f->where.y,  SCALE_FACTOR, FOOD_COLOR);
	}else{
		refresh_food(f);
	}
}

void snake_init(snake *s, food *f){
	int i;
	memset(s, 0, sizeof(snake));
	s->len = SNAKE_DEF_LEN;
	s->score = 0;
	for(i = 0; i < s->len; i++){
		s->body[i].x = WIN_WIDTH/2;
		s->body[i].y = WIN_HEIGHT/2;
	}	
	f->where.x = RANDOM(123, WIN_WIDTH);
	f->where.y = RANDOM(456, WIN_HEIGHT);
	f->life = FOOD_LIFE;
}

bool snake_eat(snake *s, food *f){
	if(s->body[0].x == f->where.x && s->body[0].y == f->where.y){
		if(s->len < SNAKE_MAX_LEN)
			s->len++;
		s->score++;
		refresh_food(f);
		return true;
	}
	return false;
}

bool snake_eat_self(snake *s){
    int i;
    for(i=1; i<s->len; i++){
		if(s->body[0].x == s->body[i].x &&  
		   s->body[0].y == s->body[i].y)
			return true;
	} 
	return false;
}

static snake s;
static food f;
static char info[32];
font_t* font;
static int record = 0;
static bool dead = true;
static int dir = LEFT;

static void event_handle(xwin_t* x, xevent_t* xev) {
	(void)x;
	if(xev->type == XEVT_IM) {
		int key = xev->value.im.value;
		if(key == 27) {//esc
			//x->terminated = true; //TODO
			return;
		}
		seed += key;

		if(key == KEY_LEFT && dir != RIGHT)
			dir = LEFT;
		else if(key == KEY_UP && dir != DOWN)
			dir = UP;
		else if(key == KEY_RIGHT && dir != LEFT)
			dir = RIGHT;
		else if(key == KEY_DOWN && dir != UP)
			dir = DOWN;
		else if(key == KEY_ENTER)
			dead = false;
	}

}

static void repaint(xwin_t* x, graph_t* g) {
	(void)x;
	graph_clear(g, BG_COLOR);
	graph_draw_text(g, 0, 0, info, font, TEXT_COLOR);
	snake_draw(g, &s, &f);
}

static void loop(void* p) {
	if(!top)
		return;

	xwin_t* xwin = (xwin_t*)p;
	if(dead == false){
		snake_move(&s, dir);
		snake_eat(&s, &f);
		dead = snake_eat_self(&s);
		if(dead == true){
			if(s.score > record){
				snprintf(info, sizeof(info), "New Record: %d", s.score);
				record = s.score;
			}else
				snprintf(info, sizeof(info), "DEAD! SCORE: %d", s.score);
			snake_init(&s, &f);
		}
		else
			snprintf(info, sizeof(info), "BEST:%d  SCORE:%d", record, s.score);
	}
	x_repaint(xwin, false);
	usleep(10000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	top = false;
	xscreen_t scr;
	x_screen_info(&scr, 0);

	x_t x;
	x_init(&x, NULL);

	xwin_t* xwin = x_open(&x, 10, 10, WIN_WIDTH*SCALE_FACTOR, WIN_HEIGHT*SCALE_FACTOR + 20, "snake", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);
	xwin->on_focus = on_focus;
	xwin->on_unfocus = on_unfocus;
	xwin->on_event = event_handle;
	xwin->on_repaint = repaint;

	font = font_by_name("7x9");
	dead = true;
	record = 0;
	dir = LEFT;

	snprintf(info, sizeof(info),  "Press to Start...");
	snake_init(&s, &f);

	x_set_visible(xwin, true);

	x.on_loop = loop;
	x_run(&x, xwin);

	x_close(xwin);
	return 0;
} 

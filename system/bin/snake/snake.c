#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <x/xclient.h>

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

void on_focus(x_t* x, void* p) {
	(void)x;
	*(int*)p = 1;
}

void on_unfocus(x_t* x, void* p) {
	(void)x;
	*(int*)p = 0;
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
			pixel_safe(g, x * scale + i, 
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

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(10, 10, WIN_WIDTH*SCALE_FACTOR, WIN_HEIGHT*SCALE_FACTOR + 20, "snake", X_STYLE_NORMAL);
	int top = 0;
	x->data = &top;
	x->on_focus = on_focus;
	x->on_unfocus = on_unfocus;

	x_set_visible(x, true);
	font_t* font = font_by_name("7x9");

	snake s;
	food f;
	int dir = LEFT;
	int record = 0;
	bool dead = true;
	char info[32];

	snprintf(info, sizeof(info),  "Press to Start...");
	snake_init(&s, &f);

	xevent_t xev;
	while(x->closed == 0) {
		int key = 0;
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_KEYB) {
				key = xev.value.keyboard.value;
				if(key == 27) //esc
					break;
				seed += key;
			}
		}
		if(top == 1) {

			if(key == 37 && dir != RIGHT)
				dir = LEFT;
			else if(key == 38 && dir != DOWN)
				dir = UP;
			else if(key == 39 && dir != LEFT)
				dir = RIGHT;
			else if(key == 40 && dir != UP)
				dir = DOWN;
			else if(key == 13)
				dead = false;

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

			graph_t* g = x_get_graph(x);
			clear(g, argb_int(BG_COLOR));
			draw_text(g, 0, 0, info, font, TEXT_COLOR);
			snake_draw(g, &s, &f);
			x_release_graph(x, g);
			x_update(x);
		}
		usleep(20000);
	}

	x_close(x);
	return 0;
} 

#ifndef TINYJSON_H
#define TINYJSON_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ewoksys/mstr.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
typedef enum bool_enum {false, true} bool;
#endif

/**====== memory functions.======*/
typedef void (*free_func_t)(void* p);

#define STATIC_STR_MAX 32

/**====== array functions. ======*/

typedef struct st_array {
	void** items;
	uint32_t max: 16;
	uint32_t size: 16;
} m_array_t;

m_array_t* array_new(void);
void array_free(m_array_t* array, free_func_t fr);
void array_init(m_array_t* array);
void array_add(m_array_t* array, void* item);
void array_add_head(m_array_t* array, void* item);
void* array_add_buf(m_array_t* array, void* s, uint32_t sz);
void* array_get(m_array_t* array, uint32_t index);
void* array_set(m_array_t* array, uint32_t index, void* p);
void* array_tail(m_array_t* array);
void* array_head(m_array_t* array);
void* array_remove(m_array_t* array, uint32_t index);
void array_del(m_array_t* array, uint32_t index, free_func_t fr);
void array_remove_all(m_array_t* array);
void array_clean(m_array_t* array, free_func_t fr);
#define array_tail(array) (((array)->items == NULL || (array)->size == 0) ? NULL: (array)->items[(array)->size-1]);

/**====== Script Lex. =======*/ 

typedef enum {
	LEX_EOF  =  0,
  LEX_ID = 256,
	LEX_INT,
  LEX_FLOAT,
  LEX_STR,
	LEX_BASIC_END
} lex_basic_type_t;

typedef struct st_lex {
	const char* data;

	int32_t data_pos;
	int32_t data_start, data_end;
	char curr_ch, next_ch;

	uint32_t tk;
	str_t* tk_str;
	int32_t tk_start, tk_end, tk_last_end;
} lex_t;

bool is_whitespace(unsigned char ch);

bool is_space(unsigned char ch);

bool is_numeric(unsigned char ch);

bool is_number(const char* cstr);

bool is_hexadecimal(unsigned char ch);

bool is_alpha(unsigned char ch);

bool is_alpha_num(const char* cstr);

void lex_get_nextch(lex_t* lex);

void lex_reset(lex_t* lex);

void lex_init(lex_t * lex, const char* input);

void lex_release(lex_t* lex);

void lex_skip_whitespace(lex_t* lex);

void lex_skip_space(lex_t* lex);

bool lex_skip_comments_line(lex_t* lex, const char* start);

bool lex_skip_comments_block(lex_t* lex, const char* start, const char* end);

void lex_token_start(lex_t* lex); 

void lex_token_end(lex_t* lex); 

void lex_get_char_token(lex_t* lex); 

void lex_get_basic_token(lex_t* lex); 

void lex_get_pos(lex_t* lex, int* line, int *col, int pos);

//script var
#define V_UNDEF  0
#define V_INT    1
#define V_FLOAT  2
#define V_STRING 3
#define V_OBJECT 4
#define V_BOOL   5
#define V_NULL   6

typedef struct st_var {
  uint32_t magic: 8; //0 for var; 1 for node
  uint32_t type:10;
  uint32_t is_array:2;
  uint32_t refs;

  uint32_t size;  // size for bytes type of value;
  void* value;
 
  struct st_var* prev; //for var list
  struct st_var* next; //for var list
  m_array_t children;
} var_t; 


//script node for var member children
typedef struct st_node {
	int16_t magic: 8; //1 for node
	int16_t be_const : 8;
	char* name;
	var_t* var;
} node_t;

node_t* node_new(const char* name, var_t* var);
void node_free(void* p);
var_t* node_replace(node_t* node, var_t* v);

void var_remove_all(var_t* var);
node_t* var_add(var_t* var, const char* name, var_t* add);
node_t* var_add_head(var_t* var, const char* name, var_t* add);
node_t* var_find(var_t* var, const char*name);
var_t* var_find_var(var_t* var, const char*name);
node_t* var_find_create(var_t* var, const char*name);
node_t* var_get(var_t* var, int32_t index);

node_t* var_array_get(var_t* var, int32_t index);
var_t* var_array_get_var(var_t* var, int32_t index);
node_t* var_array_add(var_t* var, var_t* add);
node_t* var_array_add_head(var_t* var, var_t* add);
node_t* var_array_set(var_t* var, int32_t index, var_t* set_var);
node_t* var_array_remove(var_t* var, int32_t index);
void var_array_del(var_t* var, int32_t index);
void var_array_reverse(var_t* var);
uint32_t var_array_size(var_t* var);
void var_clean(var_t* var);

var_t* var_ref(var_t* var);
void var_unref(var_t* var);

var_t* var_new(void);
var_t* var_new_array(void);
var_t* var_new_int(int i);
var_t* var_new_null(void);
var_t* var_new_bool(bool b);
var_t* var_new_obj(void*p);
var_t* var_new_float(float i);
var_t* var_new_str(const char* s);
var_t* var_new_str2(const char* s, uint32_t len);
const char* var_get_str(var_t* var);
var_t* var_set_str(var_t* var, const char* v);
int var_get_int(var_t* var);
var_t* var_set_int(var_t* var, int v);
bool var_get_bool(var_t* var);
float var_get_float(var_t* var);
var_t* var_set_float(var_t* var, float v);

void var_to_json_str(var_t*, str_t*, int);
void var_to_str(var_t*, str_t*);

node_t* find_member(var_t* obj, const char* name);
var_t* get_obj(var_t* obj, const char* name);
void* get_raw(var_t* obj, const char* name);
const char* get_str(var_t* obj, const char* name);
int get_int(var_t* obj, const char* name);
float get_float(var_t* obj, const char* name);
bool get_bool(var_t* obj, const char* name);
var_t* get_obj_member(var_t* obj, const char* name);
var_t* set_obj_member(var_t* obj, const char* name, var_t* var);

node_t* json_find(var_t* var, const char* path_name);
var_t*  json_find_var(var_t* var, const char* path_name);

extern var_t* json_parse(const char* str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

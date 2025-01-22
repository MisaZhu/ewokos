#ifndef TINYJSON_H
#define TINYJSON_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ewoksys/mstr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**====== memory functions.======*/
typedef void (*free_func_t)(void* p);

#define STATIC_STR_MAX 32

/**====== array functions. ======*/

typedef struct st_json_array {
	void** items;
	uint32_t max: 16;
	uint32_t size: 16;
} json_array_t;

json_array_t* json_array_new(void);
void json_array_free(json_array_t* array, free_func_t fr);
void json_array_init(json_array_t* array);
void json_array_add(json_array_t* array, void* item);
void json_array_add_head(json_array_t* array, void* item);
void* json_array_add_buf(json_array_t* array, void* s, uint32_t sz);
void* json_array_get(json_array_t* array, uint32_t index);
void* json_array_set(json_array_t* array, uint32_t index, void* p);
void* json_array_tail(json_array_t* array);
void* json_array_head(json_array_t* array);
void* json_array_remove(json_array_t* array, uint32_t index);
void json_array_del(json_array_t* array, uint32_t index, free_func_t fr);
void json_array_remove_all(json_array_t* array);
void json_array_clean(json_array_t* array, free_func_t fr);
#define json_array_tail(array) (((array)->items == NULL || (array)->size == 0) ? NULL: (array)->items[(array)->size-1]);

/**====== Script Lex. =======*/ 

typedef enum {
	JSON_LEX_EOF  =  0,
	JSON_LEX_ID = 256,
	JSON_LEX_INT,
	JSON_LEX_FLOAT,
	JSON_LEX_STR,
	JSON_LEX_BASIC_END
} json_lex_basic_type_t;

typedef struct st_json_lex {
	const char* data;

	int32_t data_pos;
	int32_t data_start, data_end;
	char curr_ch, next_ch;

	uint32_t tk;
	str_t* tk_str;
	int32_t tk_start, tk_end, tk_last_end;
} json_lex_t;

bool json_is_whitespace(unsigned char ch);

bool json_is_space(unsigned char ch);

bool json_is_numeric(unsigned char ch);

bool json_is_number(const char* cstr);

bool json_is_hexadecimal(unsigned char ch);

bool json_is_alpha(unsigned char ch);

bool json_is_alpha_num(const char* cstr);

void json_lex_get_nextch(json_lex_t* lex);

void json_lex_reset(json_lex_t* lex);

void json_lex_init(json_lex_t * lex, const char* input);

void json_lex_release(json_lex_t* lex);

void json_lex_skip_whitespace(json_lex_t* lex);

void json_lex_skip_space(json_lex_t* lex);

bool json_lex_skip_comments_line(json_lex_t* lex, const char* start);

bool json_lex_skip_comments_block(json_lex_t* lex, const char* start, const char* end);

void json_lex_token_start(json_lex_t* lex); 

void json_lex_token_end(json_lex_t* lex); 

void json_lex_get_char_token(json_lex_t* lex); 

void json_lex_get_basic_token(json_lex_t* lex); 

void json_lex_get_pos(json_lex_t* lex, int* line, int *col, int pos);

//script var
#define JSON_V_UNDEF  0
#define JSON_V_INT    1
#define JSON_V_FLOAT  2
#define JSON_V_STRING 3
#define JSON_V_OBJECT 4
#define JSON_V_BOOL   5
#define JSON_V_NULL   6

typedef struct st_json_var {
  uint32_t magic: 8; //0 for var; 1 for node
  uint32_t type:10;
  uint32_t json_is_array:2;
  uint32_t refs;

  uint32_t size;  // size for bytes type of value;
  void* value;
  free_func_t free_func;
 
  struct st_json_var* prev; //for var list
  struct st_json_var* next; //for var list
  json_array_t children;
} json_var_t; 


//script node for var member children
typedef struct st_json_node {
	int16_t magic: 8; //1 for node
	int16_t be_const : 8;
	char* name;
	json_var_t* var;
} json_node_t;

json_node_t* json_node_new(const char* name, json_var_t* var);
void json_node_free(void* p);
json_var_t* json_node_replace(json_node_t* node, json_var_t* v);

void json_var_remove_all(json_var_t* var);
json_node_t* json_var_add(json_var_t* var, const char* name, json_var_t* add);
json_node_t* json_var_add_head(json_var_t* var, const char* name, json_var_t* add);
json_node_t* json_var_find(json_var_t* var, const char*name);
json_var_t* json_var_find_var(json_var_t* var, const char*name);
json_node_t* json_var_find_create(json_var_t* var, const char*name);
json_node_t* json_var_get(json_var_t* var, int32_t index);

json_node_t* json_var_array_get(json_var_t* var, int32_t index);
json_var_t* json_var_array_get_var(json_var_t* var, int32_t index);
json_node_t* json_var_array_add(json_var_t* var, json_var_t* add);
json_node_t* json_var_array_add_head(json_var_t* var, json_var_t* add);
json_node_t* json_var_array_set(json_var_t* var, int32_t index, json_var_t* set_var);
json_node_t* json_var_array_remove(json_var_t* var, int32_t index);
void json_var_array_del(json_var_t* var, int32_t index);
void json_var_array_reverse(json_var_t* var);
uint32_t json_var_array_size(json_var_t* var);
void json_var_clean(json_var_t* var);

json_var_t* json_var_ref(json_var_t* var);
void json_var_unref(json_var_t* var);

json_var_t* json_var_new(void);
json_var_t* json_var_new_array(void);
json_var_t* json_var_new_int(int i);
json_var_t* json_var_new_null(void);
json_var_t* json_var_new_bool(bool b);
json_var_t* json_var_new_obj(void*p, free_func_t fr);
json_var_t* json_var_new_float(float i);
json_var_t* json_var_new_str(const char* s);
json_var_t* json_var_new_str2(const char* s, uint32_t len);
const char* json_var_json_get_str(json_var_t* var);
json_var_t* json_var_set_str(json_var_t* var, const char* v);
int json_var_json_get_int(json_var_t* var);
json_var_t* json_var_set_int(json_var_t* var, int v);
bool json_var_json_get_bool(json_var_t* var);
float json_var_json_get_float(json_var_t* var);
json_var_t* json_var_set_float(json_var_t* var, float v);

void json_var_to_json_str(json_var_t*, str_t*, int);
void json_var_to_str(json_var_t*, str_t*);

json_node_t* json_find_member(json_var_t* obj, const char* name);
json_var_t* json_get_obj(json_var_t* obj, const char* name);
void* json_get_raw(json_var_t* obj, const char* name);

const char* json_get_str_def(json_var_t* obj, const char* name, const char* def);
int json_get_int_def(json_var_t* obj, const char* name, int def);
float json_get_float_def(json_var_t* obj, const char* name, float def);
bool json_get_bool_def(json_var_t* obj, const char* name, bool def);

const char* json_get_str(json_var_t* obj, const char* name);
int json_get_int(json_var_t* obj, const char* name);
float json_get_float(json_var_t* obj, const char* name);
bool json_get_bool(json_var_t* obj, const char* name);

json_var_t* json_get_obj_member(json_var_t* obj, const char* name);
json_var_t* json_set_obj_member(json_var_t* obj, const char* name, json_var_t* var);

json_node_t* json_find(json_var_t* var, const char* path_name);
json_var_t*  json_find_var(json_var_t* var, const char* path_name);

extern json_var_t* json_parse_str(const char* str);
extern json_var_t* json_parse_file(const char* fname);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

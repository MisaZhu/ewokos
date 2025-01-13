#include "tinyjson/tinyjson.h"
#include "ewoksys/vfs.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef MRCIO_THREAD
#include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**======array functions======*/

#define ARRAY_BUF 16

inline void array_init(m_array_t* array) { 
	array->items = NULL; 
	array->size = 0; 
	array->max = 0; 
}

m_array_t* array_new() {
	m_array_t* ret = (m_array_t*)malloc(sizeof(m_array_t));
	array_init(ret);
	return ret;
}

inline void array_add(m_array_t* array, void* item) {
	uint32_t new_size = array->size + 1; 
	if(array->max <= new_size) { 
		new_size = array->size + ARRAY_BUF;
		array->items = (void**)realloc(array->items, new_size*sizeof(void*)); 
		array->max = new_size; 
	} 
	array->items[array->size] = item; 
	array->size++; 
	array->items[array->size] = NULL; 
}

inline void array_add_head(m_array_t* array, void* item) {
	uint32_t new_size = array->size + 1; 
	if(array->max <= new_size) { 
		new_size = array->size + ARRAY_BUF;
		array->items = (void**)realloc(array->items, new_size*sizeof(void*)); 
		array->max = new_size; 
	} 
	int32_t i;
	for(i=array->size; i>0; i--) {
		array->items[i] = array->items[i-1];
	}
	array->items[0] = item; 
	array->size++; 
	array->items[array->size] = NULL; 
}

void* array_add_buf(m_array_t* array, void* s, uint32_t sz) {
	void* item = malloc(sz);
	if(s != NULL)
		memcpy(item, s, sz);
	array_add(array, item);
	return item;
}

inline void* array_get(m_array_t* array, uint32_t index) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	return array->items[index];
}

inline void* array_set(m_array_t* array, uint32_t index, void* p) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	array->items[index] = p;
	return p;
}

inline void* array_head(m_array_t* array) {
	if(array->items == NULL || array->size == 0)
		return NULL;
	return array->items[0];
}

inline void* array_remove(m_array_t* array, uint32_t index) { //remove out but not free
	if(index >= array->size)
		return NULL;

	void *p = array->items[index];
	uint32_t i;
	for(i=index; (i+1)<array->size; i++) {
		array->items[i] = array->items[i+1];	
	}

	array->size--;
	array->items[array->size] = NULL;
	return p;
}

inline void array_del(m_array_t* array, uint32_t index, free_func_t fr) { // remove out and free.
	void* p = array_remove(array, index);
	if(p != NULL) {
		if(fr != NULL)
			fr(p);
		else
			free(p);
	}
}

inline void array_remove_all(m_array_t* array) { //remove all items bot not free them.
	if(array->items != NULL) {
		free(array->items);
		array->items = NULL;
	}
	array->max = array->size = 0;
}

inline void array_free(m_array_t* array, free_func_t fr) {
	array_clean(array, fr);
	free(array);
}

inline void array_clean(m_array_t* array, free_func_t fr) { //remove all items and free them.
	if(array->items != NULL) {
		uint32_t i;
		for(i=0; i<array->size; i++) {
			void* p = array->items[i];
			if(p != NULL) {
				if(fr != NULL)
					fr(p);
				else
					free(p);
			}
		}
		free(array->items);
		array->items = NULL;
	}
	array->max = array->size = 0;
}

/**======lex functions======*/

bool is_whitespace(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
		return true;
	return false;
}

bool is_space(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\r')
		return true;
	return false;
}

bool is_numeric(unsigned char ch) {
	if(ch >= '0' && ch <= '9')
		return true;
	return false;
}

bool is_number(const char* cstr) {
	int  i= 0;
	while(cstr[i] != 0) {
		if (is_numeric(cstr[i]) == false)
			return false;
		i++;
	}
	return true;
}

bool is_hexadecimal(unsigned char ch) {
	if(((ch>='0') && (ch<='9')) ||
		((ch>='a') && (ch<='f')) ||
		((ch>='A') && (ch<='F')))
			return true;
	return false;
}

bool is_alpha(unsigned char ch) {
	if(((ch>='a') && (ch<='z')) ||
		((ch>='A') && (ch<='Z')) ||
		ch == '_')
		return true;
	return false;
}

bool is_alpha_num(const char* cstr) {
	if (cstr[0] == 0){
		return true;
	}
	if (is_alpha(cstr[0]) == 0){
		return false;
	}

	int  i= 0;
	while(cstr[i] != 0) {
		if (is_alpha(cstr[i]) == false || is_numeric(cstr[i]) == true){
			return false;
		}
		i++;
	}
	return true;
}

void lex_get_nextch(lex_t* lex) {
	lex->curr_ch = lex->next_ch;
	if (lex->data_pos < lex->data_end){
		lex->next_ch = lex->data[lex->data_pos];
	}else{
		lex->next_ch = 0;
	}
	lex->data_pos++;
}

void lex_skip_whitespace(lex_t* lex) {
	while (lex->curr_ch && is_whitespace(lex->curr_ch)){
		lex_get_nextch(lex);
	}
}

void lex_skip_space(lex_t* lex) {
	while (lex->curr_ch && is_space(lex->curr_ch)){
		lex_get_nextch(lex);
	}
}

//only take first 1~2 chars from start
bool lex_skip_comments_line(lex_t* lex, const char* start) {
	if(start[0] == 0)
		return false;

	if ((lex->curr_ch==start[0] && (start[1] == 0 || lex->next_ch==start[1]))) {
		while (lex->curr_ch && lex->curr_ch!='\n'){
			lex_get_nextch(lex);
		}
		if(start[1] != 0)
			lex_get_nextch(lex);
		return true;
	}
	return false;
}

//only take first 2 chars from start and end.
bool lex_skip_comments_block(lex_t* lex, const char* start, const char* end) {
	if(start[0] == 0 || start[1] == 0 || end[0] == 0 || end[1] == 0)
		return false;

	if (lex->curr_ch==start[0] && lex->next_ch==start[1]) {
		while (lex->curr_ch && (lex->curr_ch!=end[0] || lex->next_ch!=end[1])) {
			lex_get_nextch(lex);
		}
		lex_get_nextch(lex);
		lex_get_nextch(lex);
		return true;
	}	
	return false;
}

void lex_reset(lex_t* lex) {
	lex->data_pos = lex->data_start;
	lex->tk_start   = 0;
	lex->tk_end     = 0;
	lex->tk_last_end = 0;
	lex->tk  = LEX_EOF;
	str_reset(lex->tk_str);
	lex_get_nextch(lex);
	lex_get_nextch(lex);
}

void lex_init(lex_t * lex, const char* input) {
	lex->data = input;
	lex->data_start = 0;
	lex->data_end = (int)strlen(lex->data);
	lex->tk_str = str_new("");
	lex_reset(lex);
}

void lex_release(lex_t* lex) {
	str_free(lex->tk_str);
	lex->tk_str = NULL;
}

void lex_token_start(lex_t* lex) {
	// record beginning of this token(pre-read 2 chars );
	lex->tk_start = lex->data_pos-2;
}

void lex_token_end(lex_t* lex) {
	lex->tk_last_end = lex->tk_end;
	lex->tk_end = lex->data_pos-3;
}

void lex_get_char_token(lex_t* lex) {
	lex->tk = lex->curr_ch;
	if (lex->curr_ch) 
		lex_get_nextch(lex);
}

void lex_get_basic_token(lex_t* lex) {
	// tokens
	if (is_alpha(lex->curr_ch)) { //  IDs
		while (is_alpha(lex->curr_ch) || is_numeric(lex->curr_ch)) {
			str_addc(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		lex->tk = LEX_ID;
	} else if (is_numeric(lex->curr_ch)) { // _numbers
		bool isHex = false;
		if (lex->curr_ch=='0') {
			str_addc(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		if (lex->curr_ch == 'x' || lex->curr_ch == 'X') {
			isHex = true;
			str_addc(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		lex->tk = LEX_INT;

		while (is_numeric(lex->curr_ch) || (isHex && is_hexadecimal(lex->curr_ch))) {
			str_addc(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
		}
		if (!isHex && lex->curr_ch=='.' && is_numeric(lex->next_ch)) {
			lex->tk = LEX_FLOAT;
			str_addc(lex->tk_str, '.');
			lex_get_nextch(lex);
			while (is_numeric(lex->curr_ch)) {
				str_addc(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
		}
		// do fancy e-style floating point
		if (!isHex && (lex->curr_ch=='e'||lex->curr_ch=='E')) {
			lex->tk = LEX_FLOAT;
			str_addc(lex->tk_str, lex->curr_ch);
			lex_get_nextch(lex);
			if (lex->curr_ch=='-') {
				str_addc(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
			while (is_numeric(lex->curr_ch)) {
				str_addc(lex->tk_str, lex->curr_ch);
				lex_get_nextch(lex);
			}
		}
	} else if (lex->curr_ch=='"') {
		// strings...
		lex_get_nextch(lex);
		while (lex->curr_ch && lex->curr_ch!='"') {
			if (lex->curr_ch == '\\') {
				lex_get_nextch(lex);
				switch (lex->curr_ch) {
					case 'n' : str_addc(lex->tk_str, '\n'); break;
					case 'r' : str_addc(lex->tk_str, '\r'); break;
					case 't' : str_addc(lex->tk_str, '\t'); break;
					case '"' : str_addc(lex->tk_str, '\"'); break;
					case '\\' : str_addc(lex->tk_str, '\\'); break;
					default: str_addc(lex->tk_str, lex->curr_ch);
				}
			} else {
				str_addc(lex->tk_str, lex->curr_ch);
			}
			lex_get_nextch(lex);
		}
		lex_get_nextch(lex);
		lex->tk = LEX_STR;
	}
}

void lex_get_pos(lex_t* lex, int* line, int *col, int pos) {
	if (pos<0) 
		pos= lex->tk_last_end;

	int l = 1;
	int c  = 1;
	int i;
	for (i=0; i<pos; i++) {
		char ch;
		if (i < lex->data_end){
			ch = lex->data[i];
		}else{
			ch = 0;
		}

		c++;
		if (ch=='\n') {
			l++;
			c = 1;
		}
	}
	*line = l;
	*col = c;
}

typedef enum {
  // reserved words
  LEX_R_TRUE,
  LEX_R_FALSE,
  LEX_R_NULL,
  LEX_R_UNDEFINED
} JSON_LEX_TYPES;

static void lex_js_get_js_str(lex_t* lex) {
	// js style strings 
	lex_get_nextch(lex);
	while (lex->curr_ch && lex->curr_ch!='\'') {
		if (lex->curr_ch == '\\') {
			lex_get_nextch(lex);
			switch (lex->curr_ch) {
				case 'n' : str_addc(lex->tk_str, '\n'); break;
				case 'a' : str_addc(lex->tk_str, '\a'); break;
				case 'r' : str_addc(lex->tk_str, '\r'); break;
				case 't' : str_addc(lex->tk_str, '\t'); break;
				case '\'' : str_addc(lex->tk_str, '\''); break;
				case '\\' : str_addc(lex->tk_str, '\\'); break;
				case 'x' : { // hex digits
										 char buf[3] = "??";
										 lex_get_nextch(lex);
										 buf[0] = lex->curr_ch;
										 lex_get_nextch(lex);
										 buf[1] = lex->curr_ch;
										 str_addc(lex->tk_str, (char)strtoul(buf, 0, 16));
									 } break;
				default: if (lex->curr_ch>='0' && lex->curr_ch<='7') {
									 // octal digits
									 char buf[4] = "???";
									 buf[0] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[1] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[2] = lex->curr_ch;
									 str_addc(lex->tk_str, (char)strtoul(buf, 0, 8));
								 } else
									 str_addc(lex->tk_str, lex->curr_ch);
			}
		} else {
			str_addc(lex->tk_str, lex->curr_ch);
		}
		lex_get_nextch(lex);
	}
	lex_get_nextch(lex);
	lex->tk = LEX_STR;
}

static void lex_js_get_reserved_word(lex_t *lex) {
	if (strcmp(lex->tk_str->cstr, "true") == 0)      lex->tk = LEX_R_TRUE;
	else if (strcmp(lex->tk_str->cstr, "false") == 0)     lex->tk = LEX_R_FALSE;
	else if (strcmp(lex->tk_str->cstr, "null") == 0)      lex->tk = LEX_R_NULL;
	else if (strcmp(lex->tk_str->cstr, "undefined") == 0) lex->tk = LEX_R_UNDEFINED;
}

static void lex_js_get_next_token(lex_t* lex) {
	lex->tk = LEX_EOF;
	str_reset(lex->tk_str);

	lex_skip_whitespace(lex);
	if(lex_skip_comments_line(lex, "//")) {
		lex_js_get_next_token(lex);
		return;
	}
	if(lex_skip_comments_block(lex, "/*", "*/")) {
		lex_js_get_next_token(lex);
		return;
	}

	lex_token_start(lex);
	lex_get_basic_token(lex);

	if (lex->tk == LEX_ID) { //  IDs
		lex_js_get_reserved_word(lex);
	} 
	else if(lex->tk == LEX_EOF) {
		if (lex->curr_ch=='\'') {
			// js style strings 
			lex_js_get_js_str(lex);
		} 
		else {
			lex_get_char_token(lex);
		}
	}

	lex_token_end(lex);
}

static bool lex_js_chkread(lex_t* lex, uint32_t expected_tk) {
	if (lex->tk != expected_tk) {
		return false;
	}
	lex_js_get_next_token(lex);
	return true;
}

static var_t* json_parse_factor(lex_t *l) {
	if (l->tk==LEX_R_TRUE) {
		lex_js_chkread(l, LEX_R_TRUE);
		return var_new_int(1);
	}
	else if (l->tk==LEX_R_FALSE) {
		lex_js_chkread(l, LEX_R_FALSE);
		return var_new_int(0);
	}
	else if (l->tk==LEX_R_NULL) {
		lex_js_chkread(l, LEX_R_NULL);
		return var_new();
	}
	else if (l->tk==LEX_R_UNDEFINED) {
		lex_js_chkread(l, LEX_R_UNDEFINED);
		return var_new();
	}
	else if (l->tk==LEX_INT) {
		int i = 0;
		if(strchr(l->tk_str->cstr, 'x') != NULL || strchr(l->tk_str->cstr, 'X') != NULL)
			i = strtoul(l->tk_str->cstr, 0, 16);
		else	
			i = atoi(l->tk_str->cstr);
		lex_js_chkread(l, LEX_INT);
		return var_new_int(i);
	}
	else if (l->tk==LEX_FLOAT) {
		float f = 0.0;//TODO atof(l->tk_str->cstr);
		lex_js_chkread(l, LEX_FLOAT);
		return var_new_float(f);
	}
	else if (l->tk==LEX_STR) {
		str_t* s = str_new(l->tk_str->cstr);
		lex_js_chkread(l, LEX_STR);
		var_t* ret = var_new_str(s->cstr);
		str_free(s);
		return ret;
	}
	else if (l->tk=='[') {
		/* JSON-style array */
		var_t* arr = var_new_array();
		lex_js_chkread(l, '[');
		while (l->tk != ']') {
			var_t* v = json_parse_factor(l);
			var_array_add(arr, v);
			if (l->tk != ']') 
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, ']');
		return arr;
	}
	else if (l->tk=='{') {
		lex_js_chkread(l, '{');
		var_t* obj = var_new_obj(NULL, NULL);
		while(l->tk != '}') {
			str_t* id = str_new(l->tk_str->cstr);
			if(l->tk == LEX_STR)
				lex_js_chkread(l, LEX_STR);
			else
				lex_js_chkread(l, LEX_ID);

			lex_js_chkread(l, ':');
			var_t* v = json_parse_factor(l);
			var_add(obj, id->cstr, v);
			str_free(id);
			if(l->tk != '}')
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, '}');
		return obj;
	}
	return var_new();
}

var_t* json_parse(const char* str) {
	lex_t lex;
	lex_init(&lex, str);
	lex_js_get_next_token(&lex);

	var_t* ret = json_parse_factor(&lex);
	lex_release(&lex);
	return ret;
}

var_t* json_parse_file(const char* fname) {
	int sz = 0;
	char* str = (char*)vfs_readfile(fname, &sz);
	if(str == NULL)
		return NULL;
	str[sz] = 0;
	var_t* ret = json_parse(str);
	free(str);
	return ret;
}

static var_t* var_clone(var_t* v) {
	switch(v->type) { //basic types
		case V_INT:
			return var_new_int(var_get_int(v));
		case V_FLOAT:
			return var_new_float(var_get_int(v));
		case V_STRING:
			return var_new_str(var_get_str(v));
		/*case V_BOOL:
			return var_new_bool(var_get_bool(v));
		case V_NULL:
			return var_new_null(v->vm);
		case V_UNDEF:
			return var_new(v->vm);*/
		default:
			break;
	}
	//object types
	return v; 
}

node_t* node_new(const char* name, var_t* var) {
	node_t* node = (node_t*)malloc(sizeof(node_t));
	memset(node, 0, sizeof(node_t));

	node->magic = 1;
	uint32_t len = (uint32_t)strlen(name);
	node->name = (char*)malloc(len+1);
	memcpy(node->name, name, len+1);
	if(var != NULL)
		//node->var = var_ref(var_clone(var));
		node->var = var_ref(var);
	else
		node->var = var_ref(var_new());
	return node;
}

static inline bool var_empty(var_t* var) {
	if(var == NULL)
		return true;
	return false;
}

void node_free(void* p) {
	node_t* node = (node_t*)p;
	if(node == NULL)
		return;

	free(node->name);
	if(!var_empty(node->var)) {
		var_unref(node->var);
	}
	free(node);
}

static inline bool node_empty(node_t* node) {
	if(node == NULL || var_empty(node->var))
		return true;
	return false;
}

inline var_t* node_replace(node_t* node, var_t* v) {
	var_t* old = node->var;
	//node->var = var_ref(var_clone(v));
	node->var = var_ref(v);
	var_unref(old);
	return v;
}

inline void var_remove_all(var_t* var) {
	/*free children*/
	array_clean(&var->children, node_free);
}

static inline node_t* var_find_raw(var_t* var, const char*name) {
	if(var_empty(var))
		return NULL;

	uint32_t i;
	for(i=0; i<var->children.size; i++) {
		node_t* node = (node_t*)var->children.items[i];
		if(node != NULL && node->name != NULL) {
			if(strcmp(node->name, name) == 0) {
				return node;
			}
		}
	}
	return NULL;
}

node_t* var_add(var_t* var, const char* name, var_t* add) {
	node_t* node = NULL;

	if(name[0] != 0) 
		node = var_find_raw(var, name);

	if(node == NULL) {
		node = node_new(name, add);
		array_add(&var->children, node);
	}
	else if(add != NULL)
		node_replace(node, add);

	return node;
}

node_t* var_add_head(var_t* var, const char* name, var_t* add) {
	return var_add(var, name, add);
}

inline node_t* var_find(var_t* var, const char*name) {
	node_t* node = var_find_raw(var, name);
	if(node_empty(node))
		return NULL;
	return node;
}

inline var_t* var_find_var(var_t* var, const char*name) {
	node_t* node = var_find(var, name);
	if(node == NULL)
		return NULL;
	return node->var;
}

inline node_t* var_find_create(var_t* var, const char*name) {
	node_t* n = var_find(var, name);
	if(n != NULL)
		return n;
	n = var_add(var, name, NULL);
	return n;
}

node_t* var_get(var_t* var, int32_t index) {
	node_t* node = (node_t*)array_get(&var->children, index);
	if(node_empty(node))
		return NULL;
	return node;
}

node_t* var_array_get(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;

	int32_t i;
	for(i=arr_var->children.size; i<=index; i++) {
		var_add(arr_var, "", NULL);
	}

	node_t* node = (node_t*)array_get(&arr_var->children, index);
	if(node_empty(node))
		return NULL;
	return node;
}

var_t* var_array_get_var(var_t* var, int32_t index) {
	node_t* n = var_array_get(var, index);
	if(n != NULL)
		return n->var;
	return NULL;
}

node_t* var_array_set(var_t* var, int32_t index, var_t* set_var) {
	node_t* node = var_array_get(var, index);
	if(node != NULL)
		node_replace(node, set_var);
	return node;
}

node_t* var_array_add(var_t* var, var_t* add_var) {
	node_t* ret = NULL;
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = var_add(arr_var, "", add_var);
	return ret;
}

node_t* var_array_add_head(var_t* var, var_t* add_var) {
	node_t* ret = NULL;
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = var_add_head(arr_var, "", add_var);
	return ret;
}

uint32_t var_array_size(var_t* var) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return 0;
	return arr_var->children.size;
}

void var_array_reverse(var_t* arr) {
	uint32_t sz = var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz/2; ++i) {
		node_t* n1 = var_array_get(arr, i);
		node_t* n2 = var_array_get(arr, sz-i-1);
		if(n1 != NULL && n2 != NULL) {
			var_t* tmp = n1->var;
			n1->var = n2->var;
			n2->var = tmp;
		}
	}
}

node_t* var_array_remove(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;
	return (node_t*)array_remove(&arr_var->children, index);
}

void var_array_del(var_t* var, int32_t index) {
	var_t* arr_var = var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return;
	array_del(&arr_var->children, index, node_free);
}

inline void var_clean(var_t* var) {
	if(var_empty(var))
		return;
	/*free children*/
	if(var->children.size > 0)
		var_remove_all(var);	

	/*free value*/
	if(var->value != NULL) {
		if(var->free_func != NULL)
			var->free_func(var->value);
		else
			free(var->value);
		var->value = NULL;
	}

	var_t* next = var->next; //backup next
	var_t* prev = var->prev; //backup prev
	memset(var, 0, sizeof(var_t));
	var->next = next;
	var->prev = prev;
}

static const char* get_typeof(var_t* var) {
	switch(var->type) {
		case V_UNDEF:
			return "undefined";
		case V_INT:
		case V_FLOAT:
			return "number";
		case V_BOOL: 
			return "boolean";
		case V_STRING: 
			return "string";
		case V_NULL: 
			return "null";
		case V_OBJECT: 
			return "object";
	}
	return "undefined";
}

inline var_t* var_new(void) {
	var_t* var = NULL;
    uint32_t sz = sizeof(var_t);
	var = (var_t*)malloc(sz);

	memset(var, 0, sizeof(var_t));
	var->type = V_UNDEF;
	return var;
}

static inline void var_free(void* p) {
  var_t* var = (var_t*)p;
  if(var_empty(var))
    return;

  //clean var.
  var_clean(var);
  var->type = V_UNDEF; 
  free(var);
}   
  
inline var_t* var_ref(var_t* var) {
  ++var->refs;
  return var;
} 
    
inline void var_unref(var_t* var) {
  if(var_empty(var))
    return;

  if(var->refs > 0)
    --var->refs;
    
  if(var->refs == 0) {
    /*referenced count is 0, means this variable not be referenced anymore,
    free it immediately.*/
    var_free(var);
  }
}

inline var_t* var_new_array(void) {
	var_t* var = var_new_obj(NULL, NULL);
	var->is_array = 1;
	var_t* members = var_new_obj(NULL, NULL);
	var_add(var, "_ARRAY_", members);
	return var;
}

inline var_t* var_new_int(int i) {
	var_t* var = var_new();
	var->type = V_INT;
	var->value = malloc(sizeof(int));
	*((int*)var->value) = i;
	return var;
}

inline var_t* var_new_null(void) {
	var_t* var = var_new();
	var->type = V_NULL;
	return var;
}

inline var_t* var_new_bool(bool b) {
	var_t* var = var_new();
	var->type = V_BOOL;
	var->value = malloc(sizeof(int));
	*((int*)var->value) = b;
	return var;
}

inline var_t* var_new_obj(void*p, free_func_t fr) {
	var_t* var = var_new();
	var->type = V_OBJECT;
	var->value = p;
	var->free_func = fr;
	return var;
}

inline var_t* var_new_float(float i) {
	var_t* var = var_new();
	var->type = V_FLOAT;
	var->value = malloc(sizeof(float));
	*((float*)var->value) = i;
	return var;
}

inline var_t* var_new_str(const char* s) {
	var_t* var = var_new();
	var->type = V_STRING;
	var->size = (uint32_t)strlen(s);
	var->value = malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	return var;
}

inline var_t* var_new_str2(const char* s, uint32_t len) {
	var_t* var = var_new();
	var->type = V_STRING;
	var->size = (uint32_t)strlen(s);
	if(var->size > len)
		var->size = len;
	var->value = malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	((char*)(var->value))[var->size] = 0;
	return var;
}

inline const char* var_get_str(var_t* var) {
	if(var == NULL || var->value == NULL)
		return "";
	
	return (const char*)var->value;
}

inline var_t* var_set_str(var_t* var, const char* v) {
	if(v == NULL)
		return var;

	var->type = V_STRING;
	if(var->value != NULL)
		free(var->value);
	uint32_t len = (uint32_t)strlen(v)+1;
	var->value = malloc(len);
	memcpy(var->value, v, len);
	return var;
}

inline bool var_get_bool(var_t* var) {
	if(var == NULL || var->value == NULL)
		return false;
	int i = (int)(*(int*)var->value);
	return i==0 ? false:true;
}

inline int var_get_int(var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0;
	if(var->type == V_FLOAT)	
		return (int)(*(float*)var->value);
	return *(int*)var->value;
}

inline var_t* var_set_int(var_t* var, int v) {
	var->type = V_INT;
	if(var->value != NULL)
		free(var->value);
	var->value = malloc(sizeof(int));
	*((int*)var->value) = v;
	return var;
}

inline float var_get_float(var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0.0;
	
	if(var->type == V_INT)	
		return (float)(*(int*)var->value);
	return *(float*)var->value;
}

inline var_t* var_set_float(var_t* var, float v) {
	var->type = V_FLOAT;
	if(var->value != NULL)
		free(var->value);
	var->value = malloc(sizeof(int));
	*((float*)var->value) = v;
	return var;
}

static void get_m_str(const char* str, str_t* ret) {
	str_reset(ret);
	str_addc(ret, '"');
	/*
	while(*str != 0) {
		switch (*str) {
			case '\\': str_addc(ret, "\\\\"); break;
			case '\n': str_addc(ret, "\\n"); break;
			case '\r': str_addc(ret, "\\r"); break;
			case '\a': str_addc(ret, "\\a"); break;
			case '"':  str_addc(ret, "\\\""); break;
			default: str_addc(ret, *str);
		}
		str++;
	}
	*/
	str_add(ret, str);
	str_addc(ret, '"');
}

void var_to_str(var_t* var, str_t* ret) {
	str_reset(ret);
	if(var == NULL) {
		str_cpy(ret, "undefined");
		return;
	}

	switch(var->type) {
	case V_INT:
		str_cpy(ret, str_from_int(var_get_int(var), 10));
		break;
	case V_FLOAT:
		str_cpy(ret, str_from_float(var_get_float(var)));
		break;
	case V_STRING:
		str_cpy(ret, var_get_str(var));
		break;
	case V_OBJECT:
		var_to_json_str(var, ret, 0);
		break;
	case V_BOOL:
		str_cpy(ret, var_get_int(var) == 1 ? "true":"false");
		break;
	case V_NULL:
		str_cpy(ret, "null");
		break;
	default:
		str_cpy(ret, "undefined");
		break;
	}
}

static void get_parsable_str(var_t* var, str_t* ret) {
	str_reset(ret);

	str_t* s = str_new("");
	var_to_str(var, s);
	if(var->type == V_STRING)
		get_m_str(s->cstr, ret);
	else
		str_cpy(ret, s->cstr);

	str_free(s);
}

static void append_json_spaces(str_t* ret, int level) {
	int spaces;
	for (spaces = 0; spaces<=level; ++spaces) {
        str_addc(ret, ' '); str_addc(ret, ' ');
	}
}

static bool _done_arr_inited = false;
void var_to_json_str(var_t* var, str_t* ret, int level) {
	str_reset(ret);

	uint32_t i;

	//check if done to avoid dead recursion
	static m_array_t done;
	if(level == 0) {
		if(!_done_arr_inited) {		
			array_init(&done);
			_done_arr_inited = true;
		}
		array_remove_all(&done);
	}
	if(var->type == V_OBJECT) {
		uint32_t sz = done.size;
		for(i=0; i<sz; ++i) {
			if(done.items[i] == var) { //already done before.
				str_cpy(ret, "{}");
				if(level == 0)
					array_remove_all(&done);
				return;
			}
		}
		array_add(&done, var);
	}

	if (var->is_array) {
		str_addc(ret, '[');
		uint32_t len = var_array_size(var);
		if (len>100) len=100; // we don't want to get stuck here!

		uint32_t i;
		for (i=0;i<len;i++) {
			node_t* n = var_array_get(var, i);

			str_t* s = str_new("");
			var_to_json_str(n->var, s, level);
			str_add(ret, s->cstr);
			str_free(s);

			if (i<len-1) 
				str_add(ret, ", ");
		}
		str_addc(ret, ']');
	}
	else if (var->type == V_OBJECT) {
		// children - handle with bracketed list
		int sz = (int)var->children.size;
		if(sz > 0)
			str_add(ret, "{\n");
		else
			str_add(ret, "{");

		int i;
		bool had = false;
		for(i=0; i<sz; ++i) {
			node_t* n = var_get(var, i);
			if(strcmp(n->name, "prototype") == 0)
				continue;
			if(had)
				str_add(ret, ",\n");
			had = true;
			append_json_spaces(ret, level);
			str_addc(ret, '"');
			str_add(ret, n->name);
			str_addc(ret, '"');
			str_add(ret, ": ");

			str_t* s = str_new("");
			var_to_json_str(n->var, s, level+1);
			str_add(ret, s->cstr);
			str_free(s);
		}
		if(sz > 0) {
			str_addc(ret, '\n');
		}
	
		append_json_spaces(ret, level - 1);	
		str_addc(ret, '}');
	} 
	else {
		// no children or a function... just write value directly
		str_t* s = str_new("");
		get_parsable_str(var, s);
		str_add(ret, s->cstr);
		str_free(s);
	}

	if(level == 0) {
		array_remove_all(&done);
	}
}

const char* get_str(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? "" : var_get_str(v);
}

int get_int(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_int(v);
}

bool get_bool(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_bool(v);
}

float get_float(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	return v == NULL ? 0 : var_get_float(v);
}

var_t* get_obj(var_t* var, const char* name) {
	//if(strcmp(name, THIS) == 0)
	//	return var;
	node_t* n = var_find(var, name);
	if(n == NULL)
		return NULL;
	return n->var;
}

void* get_raw(var_t* var, const char* name) {
	var_t* v = get_obj(var, name);
	if(v == NULL)
		return NULL;
	return v->value;
}

node_t* json_find(var_t* var, const char* path_name) {
	str_t *name = str_new("");
	bool end = false;
	node_t* node = NULL;
	while(!end) {
		char c = *path_name++;
		if(c != 0 && c != '/') {
			str_addc(name, c);
			continue;
		}

		if(name->len > 0) {
			node = var_find(var, name->cstr);
			if(node == NULL)
				break;
			var = node->var; 
		}

		str_reset(name);
		if(c == 0)
			end = true;
	}
	str_free(name);
	return node;
}

var_t*  json_find_var(var_t* var, const char* path_name) {
	node_t* node = json_find(var, path_name);
	if(node == NULL)
		return NULL;
	return node->var;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


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

inline void json_array_init(json_array_t* array) { 
	array->items = NULL; 
	array->size = 0; 
	array->max = 0; 
}

json_array_t* json_array_new() {
	json_array_t* ret = (json_array_t*)malloc(sizeof(json_array_t));
	json_array_init(ret);
	return ret;
}

inline void json_array_add(json_array_t* array, void* item) {
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

inline void json_array_add_head(json_array_t* array, void* item) {
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

void* json_array_add_buf(json_array_t* array, void* s, uint32_t sz) {
	void* item = malloc(sz);
	if(s != NULL)
		memcpy(item, s, sz);
	json_array_add(array, item);
	return item;
}

inline void* json_array_get(json_array_t* array, uint32_t index) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	return array->items[index];
}

inline void* json_array_set(json_array_t* array, uint32_t index, void* p) {
	if(array->items == NULL || index >= array->size)
		return NULL;
	array->items[index] = p;
	return p;
}

inline void* json_array_head(json_array_t* array) {
	if(array->items == NULL || array->size == 0)
		return NULL;
	return array->items[0];
}

inline void* json_array_remove(json_array_t* array, uint32_t index) { //remove out but not free
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

inline void json_array_del(json_array_t* array, uint32_t index, free_func_t fr) { // remove out and free.
	void* p = json_array_remove(array, index);
	if(p != NULL) {
		if(fr != NULL)
			fr(p);
		else
			free(p);
	}
}

inline void json_array_remove_all(json_array_t* array) { //remove all items bot not free them.
	if(array->items != NULL) {
		free(array->items);
		array->items = NULL;
	}
	array->max = array->size = 0;
}

inline void json_array_free(json_array_t* array, free_func_t fr) {
	json_array_clean(array, fr);
	free(array);
}

inline void json_array_clean(json_array_t* array, free_func_t fr) { //remove all items and free them.
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

bool json_is_whitespace(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
		return true;
	return false;
}

bool json_is_space(unsigned char ch) {
	if(ch == ' ' || ch == '\t' || ch == '\r')
		return true;
	return false;
}

bool json_is_numeric(unsigned char ch) {
	if(ch >= '0' && ch <= '9' || ch == '-')
		return true;
	return false;
}

bool json_is_number(const char* cstr) {
	int  i= 0;
	while(cstr[i] != 0) {
		if (json_is_numeric(cstr[i]) == false)
			return false;
		i++;
	}
	return true;
}

bool json_is_hexadecimal(unsigned char ch) {
	if(((ch>='0') && (ch<='9')) ||
		((ch>='a') && (ch<='f')) ||
		((ch>='A') && (ch<='F')))
			return true;
	return false;
}

bool json_is_alpha(unsigned char ch) {
	if(((ch>='a') && (ch<='z')) ||
		((ch>='A') && (ch<='Z')) ||
		ch == '_')
		return true;
	return false;
}

bool json_is_alpha_num(const char* cstr) {
	if (cstr[0] == 0){
		return true;
	}
	if (json_is_alpha(cstr[0]) == 0){
		return false;
	}

	int  i= 0;
	while(cstr[i] != 0) {
		if (json_is_alpha(cstr[i]) == false || json_is_numeric(cstr[i]) == true){
			return false;
		}
		i++;
	}
	return true;
}

void json_lex_get_nextch(json_lex_t* lex) {
	lex->curr_ch = lex->next_ch;
	if (lex->data_pos < lex->data_end){
		lex->next_ch = lex->data[lex->data_pos];
	}else{
		lex->next_ch = 0;
	}
	lex->data_pos++;
}

void json_lex_skip_whitespace(json_lex_t* lex) {
	while (lex->curr_ch && json_is_whitespace(lex->curr_ch)){
		json_lex_get_nextch(lex);
	}
}

void json_lex_skip_space(json_lex_t* lex) {
	while (lex->curr_ch && json_is_space(lex->curr_ch)){
		json_lex_get_nextch(lex);
	}
}

//only take first 1~2 chars from start
bool json_lex_skip_comments_line(json_lex_t* lex, const char* start) {
	if(start[0] == 0)
		return false;

	if ((lex->curr_ch==start[0] && (start[1] == 0 || lex->next_ch==start[1]))) {
		while (lex->curr_ch && lex->curr_ch!='\n'){
			json_lex_get_nextch(lex);
		}
		if(start[1] != 0)
			json_lex_get_nextch(lex);
		return true;
	}
	return false;
}

//only take first 2 chars from start and end.
bool json_lex_skip_comments_block(json_lex_t* lex, const char* start, const char* end) {
	if(start[0] == 0 || start[1] == 0 || end[0] == 0 || end[1] == 0)
		return false;

	if (lex->curr_ch==start[0] && lex->next_ch==start[1]) {
		while (lex->curr_ch && (lex->curr_ch!=end[0] || lex->next_ch!=end[1])) {
			json_lex_get_nextch(lex);
		}
		json_lex_get_nextch(lex);
		json_lex_get_nextch(lex);
		return true;
	}	
	return false;
}

void json_lex_reset(json_lex_t* lex) {
	lex->data_pos = lex->data_start;
	lex->tk_start   = 0;
	lex->tk_end     = 0;
	lex->tk_last_end = 0;
	lex->tk  = JSON_LEX_EOF;
	str_reset(lex->tk_str);
	json_lex_get_nextch(lex);
	json_lex_get_nextch(lex);
}

void json_lex_init(json_lex_t * lex, const char* input) {
	lex->data = input;
	lex->data_start = 0;
	lex->data_end = (int)strlen(lex->data);
	lex->tk_str = str_new("");
	json_lex_reset(lex);
}

void json_lex_release(json_lex_t* lex) {
	str_free(lex->tk_str);
	lex->tk_str = NULL;
}

void json_lex_token_start(json_lex_t* lex) {
	// record beginning of this token(pre-read 2 chars );
	lex->tk_start = lex->data_pos-2;
}

void json_lex_token_end(json_lex_t* lex) {
	lex->tk_last_end = lex->tk_end;
	lex->tk_end = lex->data_pos-3;
}

void json_lex_get_char_token(json_lex_t* lex) {
	lex->tk = lex->curr_ch;
	if (lex->curr_ch) 
		json_lex_get_nextch(lex);
}

void json_lex_get_basic_token(json_lex_t* lex) {
	// tokens
	if (json_is_alpha(lex->curr_ch)) { //  IDs
		while (json_is_alpha(lex->curr_ch) || json_is_numeric(lex->curr_ch)) {
			str_addc(lex->tk_str, lex->curr_ch);
			json_lex_get_nextch(lex);
		}
		lex->tk = JSON_LEX_ID;
	} else if (json_is_numeric(lex->curr_ch)) { // _numbers
		bool isHex = false;
		if (lex->curr_ch=='0') {
			str_addc(lex->tk_str, lex->curr_ch);
			json_lex_get_nextch(lex);
		}
		if (lex->curr_ch == 'x' || lex->curr_ch == 'X') {
			isHex = true;
			str_addc(lex->tk_str, lex->curr_ch);
			json_lex_get_nextch(lex);
		}
		lex->tk = JSON_LEX_INT;

		while (json_is_numeric(lex->curr_ch) || (isHex && json_is_hexadecimal(lex->curr_ch))) {
			str_addc(lex->tk_str, lex->curr_ch);
			json_lex_get_nextch(lex);
		}
		if (!isHex && lex->curr_ch=='.' && json_is_numeric(lex->next_ch)) {
			lex->tk = JSON_LEX_FLOAT;
			str_addc(lex->tk_str, '.');
			json_lex_get_nextch(lex);
			while (json_is_numeric(lex->curr_ch)) {
				str_addc(lex->tk_str, lex->curr_ch);
				json_lex_get_nextch(lex);
			}
		}
		// do fancy e-style floating point
		if (!isHex && (lex->curr_ch=='e'||lex->curr_ch=='E')) {
			lex->tk = JSON_LEX_FLOAT;
			str_addc(lex->tk_str, lex->curr_ch);
			json_lex_get_nextch(lex);
			if (lex->curr_ch=='-') {
				str_addc(lex->tk_str, lex->curr_ch);
				json_lex_get_nextch(lex);
			}
			while (json_is_numeric(lex->curr_ch)) {
				str_addc(lex->tk_str, lex->curr_ch);
				json_lex_get_nextch(lex);
			}
		}
	} else if (lex->curr_ch=='"') {
		// strings...
		json_lex_get_nextch(lex);
		while (lex->curr_ch && lex->curr_ch!='"') {
			if (lex->curr_ch == '\\') {
				json_lex_get_nextch(lex);
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
			json_lex_get_nextch(lex);
		}
		json_lex_get_nextch(lex);
		lex->tk = JSON_LEX_STR;
	}
}

void json_lex_get_pos(json_lex_t* lex, int* line, int *col, int pos) {
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
  JSON_LEX_R_TRUE,
  JSON_LEX_R_FALSE,
  JSON_LEX_R_NULL,
  JSON_LEX_R_UNDEFINED
} JSON_JSON_LEX_TYPES;

static void json_lex_js_get_js_str(json_lex_t* lex) {
	// js style strings 
	json_lex_get_nextch(lex);
	while (lex->curr_ch && lex->curr_ch!='\'') {
		if (lex->curr_ch == '\\') {
			json_lex_get_nextch(lex);
			switch (lex->curr_ch) {
				case 'n' : str_addc(lex->tk_str, '\n'); break;
				case 'a' : str_addc(lex->tk_str, '\a'); break;
				case 'r' : str_addc(lex->tk_str, '\r'); break;
				case 't' : str_addc(lex->tk_str, '\t'); break;
				case '\'' : str_addc(lex->tk_str, '\''); break;
				case '\\' : str_addc(lex->tk_str, '\\'); break;
				case 'x' : { // hex digits
										 char buf[3] = "??";
										 json_lex_get_nextch(lex);
										 buf[0] = lex->curr_ch;
										 json_lex_get_nextch(lex);
										 buf[1] = lex->curr_ch;
										 str_addc(lex->tk_str, (char)strtoul(buf, 0, 16));
									 } break;
				default: if (lex->curr_ch>='0' && lex->curr_ch<='7') {
									 // octal digits
									 char buf[4] = "???";
									 buf[0] = lex->curr_ch;
									 json_lex_get_nextch(lex);
									 buf[1] = lex->curr_ch;
									 json_lex_get_nextch(lex);
									 buf[2] = lex->curr_ch;
									 str_addc(lex->tk_str, (char)strtoul(buf, 0, 8));
								 } else
									 str_addc(lex->tk_str, lex->curr_ch);
			}
		} else {
			str_addc(lex->tk_str, lex->curr_ch);
		}
		json_lex_get_nextch(lex);
	}
	json_lex_get_nextch(lex);
	lex->tk = JSON_LEX_STR;
}

static void json_lex_js_get_reserved_word(json_lex_t *lex) {
	if (strcmp(lex->tk_str->cstr, "true") == 0)      lex->tk = JSON_LEX_R_TRUE;
	else if (strcmp(lex->tk_str->cstr, "false") == 0)     lex->tk = JSON_LEX_R_FALSE;
	else if (strcmp(lex->tk_str->cstr, "null") == 0)      lex->tk = JSON_LEX_R_NULL;
	else if (strcmp(lex->tk_str->cstr, "undefined") == 0) lex->tk = JSON_LEX_R_UNDEFINED;
}

static void json_lex_js_get_next_token(json_lex_t* lex) {
	lex->tk = JSON_LEX_EOF;
	str_reset(lex->tk_str);

	json_lex_skip_whitespace(lex);
	if(json_lex_skip_comments_line(lex, "//")) {
		json_lex_js_get_next_token(lex);
		return;
	}
	if(json_lex_skip_comments_block(lex, "/*", "*/")) {
		json_lex_js_get_next_token(lex);
		return;
	}

	json_lex_token_start(lex);
	json_lex_get_basic_token(lex);

	if (lex->tk == JSON_LEX_ID) { //  IDs
		json_lex_js_get_reserved_word(lex);
	} 
	else if(lex->tk == JSON_LEX_EOF) {
		if (lex->curr_ch=='\'') {
			// js style strings 
			json_lex_js_get_js_str(lex);
		} 
		else {
			json_lex_get_char_token(lex);
		}
	}

	json_lex_token_end(lex);
}

static bool json_lex_js_chkread(json_lex_t* lex, uint32_t expected_tk) {
	if (lex->tk != expected_tk) {
		return false;
	}
	json_lex_js_get_next_token(lex);
	return true;
}

static json_var_t* json_parse_factor(json_lex_t *l) {
	if (l->tk==JSON_LEX_R_TRUE) {
		json_lex_js_chkread(l, JSON_LEX_R_TRUE);
		return json_var_new_int(1);
	}
	else if (l->tk==JSON_LEX_R_FALSE) {
		json_lex_js_chkread(l, JSON_LEX_R_FALSE);
		return json_var_new_int(0);
	}
	else if (l->tk==JSON_LEX_R_NULL) {
		json_lex_js_chkread(l, JSON_LEX_R_NULL);
		return json_var_new();
	}
	else if (l->tk==JSON_LEX_R_UNDEFINED) {
		json_lex_js_chkread(l, JSON_LEX_R_UNDEFINED);
		return json_var_new();
	}
	else if (l->tk==JSON_LEX_INT) {
		int i = 0;
		if(strchr(l->tk_str->cstr, 'x') != NULL || strchr(l->tk_str->cstr, 'X') != NULL)
			i = strtoul(l->tk_str->cstr, 0, 16);
		else	
			i = atoi(l->tk_str->cstr);
		json_lex_js_chkread(l, JSON_LEX_INT);
		return json_var_new_int(i);
	}
	else if (l->tk==JSON_LEX_FLOAT) {
		float f = atof(l->tk_str->cstr);
		json_lex_js_chkread(l, JSON_LEX_FLOAT);
		return json_var_new_float(f);
	}
	else if (l->tk==JSON_LEX_STR) {
		str_t* s = str_new(l->tk_str->cstr);
		json_lex_js_chkread(l, JSON_LEX_STR);
		json_var_t* ret = json_var_new_str(s->cstr);
		str_free(s);
		return ret;
	}
	else if (l->tk=='[') {
		/* JSON-style array */
		json_var_t* arr = json_var_new_array();
		json_lex_js_chkread(l, '[');
		while (l->tk != ']') {
			json_var_t* v = json_parse_factor(l);
			json_var_array_add(arr, v);
			if (l->tk != ']') 
				json_lex_js_chkread(l, ',');
		}
		json_lex_js_chkread(l, ']');
		return arr;
	}
	else if (l->tk=='{') {
		json_lex_js_chkread(l, '{');
		json_var_t* obj = json_var_new_obj(NULL, NULL);
		while(l->tk != '}') {
			str_t* id = str_new(l->tk_str->cstr);
			if(l->tk == JSON_LEX_STR)
				json_lex_js_chkread(l, JSON_LEX_STR);
			else
				json_lex_js_chkread(l, JSON_LEX_ID);

			json_lex_js_chkread(l, ':');
			json_var_t* v = json_parse_factor(l);
			json_var_add(obj, id->cstr, v);
			str_free(id);
			if(l->tk != '}')
				json_lex_js_chkread(l, ',');
		}
		json_lex_js_chkread(l, '}');
		return obj;
	}
	return json_var_new();
}

json_var_t* json_parse_str(const char* str) {
	json_lex_t lex;
	json_lex_init(&lex, str);
	json_lex_js_get_next_token(&lex);

	json_var_t* ret = json_parse_factor(&lex);
	json_lex_release(&lex);
	return ret;
}

json_var_t* json_parse_file(const char* fname) {
	int sz = 0;
	char* str = (char*)vfs_readfile(fname, &sz);
	if(str == NULL)
		return NULL;
	str[sz] = 0;
	json_var_t* ret = json_parse_str(str);
	free(str);
	return ret;
}

static json_var_t* json_var_clone(json_var_t* v) {
	switch(v->type) { //basic types
		case JSON_V_INT:
			return json_var_new_int(json_var_get_int(v));
		case JSON_V_FLOAT:
			return json_var_new_float(json_var_get_int(v));
		case JSON_V_STRING:
			return json_var_new_str(json_var_get_str(v));
		/*case JSON_V_BOOL:
			return json_var_new_bool(json_var_get_bool(v));
		case JSON_V_NULL:
			return json_var_new_null(v->vm);
		case JSON_V_UNDEF:
			return json_var_new(v->vm);*/
		default:
			break;
	}
	//object types
	return v; 
}

json_node_t* json_node_new(const char* name, json_var_t* var) {
	json_node_t* node = (json_node_t*)malloc(sizeof(json_node_t));
	memset(node, 0, sizeof(json_node_t));

	node->magic = 1;
	uint32_t len = (uint32_t)strlen(name);
	node->name = (char*)malloc(len+1);
	memcpy(node->name, name, len+1);
	if(var != NULL)
		//node->var = json_var_ref(json_var_clone(var));
		node->var = json_var_ref(var);
	else
		node->var = json_var_ref(json_var_new());
	return node;
}

static inline bool json_var_empty(json_var_t* var) {
	if(var == NULL)
		return true;
	return false;
}

void json_node_free(void* p) {
	json_node_t* node = (json_node_t*)p;
	if(node == NULL)
		return;

	free(node->name);
	if(!json_var_empty(node->var)) {
		json_var_unref(node->var);
	}
	free(node);
}

static inline bool json_node_empty(json_node_t* node) {
	if(node == NULL || json_var_empty(node->var))
		return true;
	return false;
}

inline json_var_t* json_node_replace(json_node_t* node, json_var_t* v) {
	json_var_t* old = node->var;
	//node->var = json_var_ref(json_var_clone(v));
	node->var = json_var_ref(v);
	json_var_unref(old);
	return v;
}

inline void json_var_remove_all(json_var_t* var) {
	/*free children*/
	json_array_clean(&var->children, json_node_free);
}

static inline json_node_t* json_var_find_raw(json_var_t* var, const char*name) {
	if(json_var_empty(var))
		return NULL;

	uint32_t i;
	for(i=0; i<var->children.size; i++) {
		json_node_t* node = (json_node_t*)var->children.items[i];
		if(node != NULL && node->name != NULL) {
			if(strcmp(node->name, name) == 0) {
				return node;
			}
		}
	}
	return NULL;
}

json_node_t* json_var_add(json_var_t* var, const char* name, json_var_t* add) {
	json_node_t* node = NULL;

	if(name[0] != 0) 
		node = json_var_find_raw(var, name);

	if(node == NULL) {
		node = json_node_new(name, add);
		json_array_add(&var->children, node);
	}
	else if(add != NULL)
		json_node_replace(node, add);

	return node;
}

json_node_t* json_var_add_head(json_var_t* var, const char* name, json_var_t* add) {
	return json_var_add(var, name, add);
}

inline json_node_t* json_var_find(json_var_t* var, const char*name) {
	json_node_t* node = json_var_find_raw(var, name);
	if(json_node_empty(node))
		return NULL;
	return node;
}

inline json_var_t* json_var_find_var(json_var_t* var, const char*name) {
	json_node_t* node = json_var_find(var, name);
	if(node == NULL)
		return NULL;
	return node->var;
}

inline json_node_t* json_var_find_create(json_var_t* var, const char*name) {
	json_node_t* n = json_var_find(var, name);
	if(n != NULL)
		return n;
	n = json_var_add(var, name, NULL);
	return n;
}

json_node_t* json_var_get(json_var_t* var, int32_t index) {
	json_node_t* node = (json_node_t*)json_array_get(&var->children, index);
	if(json_node_empty(node))
		return NULL;
	return node;
}

json_node_t* json_var_array_get(json_var_t* var, int32_t index) {
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;

	int32_t i;
	for(i=arr_var->children.size; i<=index; i++) {
		json_var_add(arr_var, "", NULL);
	}

	json_node_t* node = (json_node_t*)json_array_get(&arr_var->children, index);
	if(json_node_empty(node))
		return NULL;
	return node;
}

json_var_t* json_var_array_get_var(json_var_t* var, int32_t index) {
	json_node_t* n = json_var_array_get(var, index);
	if(n != NULL)
		return n->var;
	return NULL;
}

json_node_t* json_var_array_set(json_var_t* var, int32_t index, json_var_t* set_var) {
	json_node_t* node = json_var_array_get(var, index);
	if(node != NULL)
		json_node_replace(node, set_var);
	return node;
}

json_node_t* json_var_array_add(json_var_t* var, json_var_t* add_var) {
	json_node_t* ret = NULL;
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = json_var_add(arr_var, "", add_var);
	return ret;
}

json_node_t* json_var_array_add_head(json_var_t* var, json_var_t* add_var) {
	json_node_t* ret = NULL;
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var != NULL)
		ret = json_var_add_head(arr_var, "", add_var);
	return ret;
}

uint32_t json_var_array_size(json_var_t* var) {
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return 0;
	return arr_var->children.size;
}

void json_var_array_reverse(json_var_t* arr) {
	uint32_t sz = json_var_array_size(arr);
	uint32_t i;
	for(i=0; i<sz/2; ++i) {
		json_node_t* n1 = json_var_array_get(arr, i);
		json_node_t* n2 = json_var_array_get(arr, sz-i-1);
		if(n1 != NULL && n2 != NULL) {
			json_var_t* tmp = n1->var;
			n1->var = n2->var;
			n2->var = tmp;
		}
	}
}

json_node_t* json_var_array_remove(json_var_t* var, int32_t index) {
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return NULL;
	return (json_node_t*)json_array_remove(&arr_var->children, index);
}

void json_var_array_del(json_var_t* var, int32_t index) {
	json_var_t* arr_var = json_var_find_var(var, "_ARRAY_");
	if(arr_var == NULL)
		return;
	json_array_del(&arr_var->children, index, json_node_free);
}

inline void json_var_clean(json_var_t* var) {
	if(json_var_empty(var))
		return;
	/*free children*/
	if(var->children.size > 0)
		json_var_remove_all(var);	

	/*free value*/
	if(var->value != NULL) {
		if(var->free_func != NULL)
			var->free_func(var->value);
		else
			free(var->value);
		var->value = NULL;
	}

	json_var_t* next = var->next; //backup next
	json_var_t* prev = var->prev; //backup prev
	memset(var, 0, sizeof(json_var_t));
	var->next = next;
	var->prev = prev;
}

static const char* get_typeof(json_var_t* var) {
	switch(var->type) {
		case JSON_V_UNDEF:
			return "undefined";
		case JSON_V_INT:
		case JSON_V_FLOAT:
			return "number";
		case JSON_V_BOOL: 
			return "boolean";
		case JSON_V_STRING: 
			return "string";
		case JSON_V_NULL: 
			return "null";
		case JSON_V_OBJECT: 
			return "object";
	}
	return "undefined";
}

inline json_var_t* json_var_new(void) {
	json_var_t* var = NULL;
    uint32_t sz = sizeof(json_var_t);
	var = (json_var_t*)malloc(sz);

	memset(var, 0, sizeof(json_var_t));
	var->type = JSON_V_UNDEF;
	return var;
}

static inline void json_var_free(void* p) {
  json_var_t* var = (json_var_t*)p;
  if(json_var_empty(var))
    return;

  //clean var.
  json_var_clean(var);
  var->type = JSON_V_UNDEF; 
  free(var);
}   
  
inline json_var_t* json_var_ref(json_var_t* var) {
  ++var->refs;
  return var;
} 
    
inline void json_var_unref(json_var_t* var) {
  if(json_var_empty(var))
    return;

  if(var->refs > 0)
    --var->refs;
    
  if(var->refs == 0) {
    /*referenced count is 0, means this variable not be referenced anymore,
    free it immediately.*/
    json_var_free(var);
  }
}

inline json_var_t* json_var_new_array(void) {
	json_var_t* var = json_var_new_obj(NULL, NULL);
	var->json_is_array = 1;
	json_var_t* members = json_var_new_obj(NULL, NULL);
	json_var_add(var, "_ARRAY_", members);
	return var;
}

inline json_var_t* json_var_new_int(int i) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_INT;
	var->value = malloc(sizeof(int));
	*((int*)var->value) = i;
	return var;
}

inline json_var_t* json_var_new_null(void) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_NULL;
	return var;
}

inline json_var_t* json_var_new_bool(bool b) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_BOOL;
	var->value = malloc(sizeof(int));
	*((int*)var->value) = b;
	return var;
}

inline json_var_t* json_var_new_obj(void*p, free_func_t fr) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_OBJECT;
	var->value = p;
	var->free_func = fr;
	return var;
}

inline json_var_t* json_var_new_float(float i) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_FLOAT;
	var->value = malloc(sizeof(float));
	*((float*)var->value) = i;
	return var;
}

inline json_var_t* json_var_new_str(const char* s) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_STRING;
	var->size = (uint32_t)strlen(s);
	var->value = malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	return var;
}

inline json_var_t* json_var_new_str2(const char* s, uint32_t len) {
	json_var_t* var = json_var_new();
	var->type = JSON_V_STRING;
	var->size = (uint32_t)strlen(s);
	if(var->size > len)
		var->size = len;
	var->value = malloc(var->size + 1);
	memcpy(var->value, s, var->size + 1);
	((char*)(var->value))[var->size] = 0;
	return var;
}

inline const char* json_var_get_str(json_var_t* var) {
	if(var == NULL || var->value == NULL)
		return "";
	
	return (const char*)var->value;
}

inline json_var_t* json_var_set_str(json_var_t* var, const char* v) {
	if(v == NULL)
		return var;

	var->type = JSON_V_STRING;
	if(var->value != NULL)
		free(var->value);
	uint32_t len = (uint32_t)strlen(v)+1;
	var->value = malloc(len);
	memcpy(var->value, v, len);
	return var;
}

inline bool json_var_get_bool(json_var_t* var) {
	if(var == NULL || var->value == NULL)
		return false;
	int i = (int)(*(int*)var->value);
	return i==0 ? false:true;
}

inline int json_var_get_int(json_var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0;

	if(var->type == JSON_V_FLOAT) {
		return (int)(*(float*)var->value);
	}
	else if(var->type == JSON_V_STRING)	 {
		const char* v = (const char*)var->value;
		if(strlen(v) > 2 &&
				v[0] == '0' &&
				(v[1] == 'x' || v[1] == 'X'))
			return strtoul(v, 0, 16);
		else	
			return atoi(v);
	}
	return *(int*)var->value;
}

inline json_var_t* json_var_set_int(json_var_t* var, int v) {
	var->type = JSON_V_INT;
	if(var->value != NULL)
		free(var->value);
	var->value = malloc(sizeof(int));
	*((int*)var->value) = v;
	return var;
}

inline float json_var_get_float(json_var_t* var) {
	if(var == NULL || var->value == NULL)
		return 0.0;
	
	if(var->type == JSON_V_INT)	
		return (float)(*(int*)var->value);
	return *(float*)var->value;
}

inline json_var_t* json_var_set_float(json_var_t* var, float v) {
	var->type = JSON_V_FLOAT;
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

void json_var_to_str(json_var_t* var, str_t* ret) {
	str_reset(ret);
	if(var == NULL) {
		str_cpy(ret, "undefined");
		return;
	}

	switch(var->type) {
	case JSON_V_INT:
		str_cpy(ret, str_from_int(json_var_get_int(var), 10));
		break;
	case JSON_V_FLOAT:
		str_cpy(ret, str_from_float(json_var_get_float(var)));
		break;
	case JSON_V_STRING:
		str_cpy(ret, json_var_get_str(var));
		break;
	case JSON_V_OBJECT:
		json_var_to_json_str(var, ret, 0);
		break;
	case JSON_V_BOOL:
		str_cpy(ret, json_var_get_int(var) == 1 ? "true":"false");
		break;
	case JSON_V_NULL:
		str_cpy(ret, "null");
		break;
	default:
		str_cpy(ret, "undefined");
		break;
	}
}

inline str_t* json_var_to_new_str(json_var_t* var) {
	str_t* ret = str_new("");
	json_var_to_str(var, ret);
	return ret;
}

inline char* json_var_to_cstr(json_var_t* var) {
	str_t* ret = str_new("");
	json_var_to_str(var, ret);
	return str_detach(ret);
}

static void get_parsable_str(json_var_t* var, str_t* ret) {
	str_reset(ret);

	str_t* s = str_new("");
	json_var_to_str(var, s);
	if(var->type == JSON_V_STRING)
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
void json_var_to_json_str(json_var_t* var, str_t* ret, int level) {
	str_reset(ret);

	uint32_t i;

	//check if done to avoid dead recursion
	static json_array_t done;
	if(level == 0) {
		if(!_done_arr_inited) {		
			json_array_init(&done);
			_done_arr_inited = true;
		}
		json_array_remove_all(&done);
	}
	if(var->type == JSON_V_OBJECT) {
		uint32_t sz = done.size;
		for(i=0; i<sz; ++i) {
			if(done.items[i] == var) { //already done before.
				str_cpy(ret, "{}");
				if(level == 0)
					json_array_remove_all(&done);
				return;
			}
		}
		json_array_add(&done, var);
	}

	if (var->json_is_array) {
		str_addc(ret, '[');
		uint32_t len = json_var_array_size(var);
		if (len>100) len=100; // we don't want to get stuck here!

		uint32_t i;
		for (i=0;i<len;i++) {
			json_node_t* n = json_var_array_get(var, i);

			str_t* s = str_new("");
			json_var_to_json_str(n->var, s, level);
			str_add(ret, s->cstr);
			str_free(s);

			if (i<len-1) 
				str_add(ret, ", ");
		}
		str_addc(ret, ']');
	}
	else if (var->type == JSON_V_OBJECT) {
		// children - handle with bracketed list
		int sz = (int)var->children.size;
		if(sz > 0)
			str_add(ret, "{\n");
		else
			str_add(ret, "{");

		int i;
		bool had = false;
		for(i=0; i<sz; ++i) {
			json_node_t* n = json_var_get(var, i);
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
			json_var_to_json_str(n->var, s, level+1);
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
		json_array_remove_all(&done);
	}
}

const char* json_get_str_def(json_var_t* var, const char* name, const char* def) {
	if(var == NULL)
		return def;

	json_var_t* v = json_get_obj(var, name);
	return v == NULL ? def : json_var_get_str(v);
}

int json_get_int_def(json_var_t* var, const char* name, int def) {
	if(var == NULL)
		return def;

	json_var_t* v = json_get_obj(var, name);
	return v == NULL ? def : json_var_get_int(v);
}

bool json_get_bool_def(json_var_t* var, const char* name, bool def) {
	if(var == NULL)
		return def;

	json_var_t* v = json_get_obj(var, name);
	return v == NULL ? def : json_var_get_bool(v);
}

float json_get_float_def(json_var_t* var, const char* name, float def) {
	if(var == NULL)
		return def;

	json_var_t* v = json_get_obj(var, name);
	return v == NULL ? def : json_var_get_float(v);
}

inline const char* json_get_str(json_var_t* var, const char* name) {
	return json_get_str_def(var, name, "");
}

inline int json_get_int(json_var_t* var, const char* name) {
	return json_get_int_def(var, name, 0);
}

inline bool json_get_bool(json_var_t* var, const char* name) {
	return json_get_bool_def(var, name, false);
}

inline float json_get_float(json_var_t* var, const char* name) {
	return json_get_float_def(var, name, 0.0);
}

json_var_t* json_get_obj(json_var_t* var, const char* name) {
	//if(strcmp(name, THIS) == 0)
	//	return var;
	json_node_t* n = json_var_find(var, name);
	if(n == NULL)
		return NULL;
	return n->var;
}

void* json_get_raw(json_var_t* var, const char* name) {
	json_var_t* v = json_get_obj(var, name);
	if(v == NULL)
		return NULL;
	return v->value;
}

json_node_t* json_find(json_var_t* var, const char* path_name) {
	str_t *name = str_new("");
	bool end = false;
	json_node_t* node = NULL;
	while(!end) {
		char c = *path_name++;
		if(c != 0 && c != '/') {
			str_addc(name, c);
			continue;
		}

		if(name->len > 0) {
			node = json_var_find(var, name->cstr);
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

json_var_t*  json_find_var(json_var_t* var, const char* path_name) {
	json_node_t* node = json_find(var, path_name);
	if(node == NULL)
		return NULL;
	return node->var;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif

#include "mario/mario_json.h"
#include "mario/mario_lex.h"
#include "mario/mario_vm.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
  // reserved words
#define LEX_R_LIST_START LEX_R_IF
  LEX_R_FUNCTION,
  LEX_R_TRUE,
  LEX_R_FALSE,
  LEX_R_NULL,
  LEX_R_UNDEFINED,
  LEX_R_LIST_END /* always the last entry */
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
										 str_addc(lex->tk_str, (char)atoi_base(buf, 16));
									 } break;
				default: if (lex->curr_ch>='0' && lex->curr_ch<='7') {
									 // octal digits
									 char buf[4] = "???";
									 buf[0] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[1] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[2] = lex->curr_ch;
									 str_addc(lex->tk_str, (char)atoi_base(buf, 8));
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
	if (strcmp(lex->tk_str->cstr, "function") == 0)  lex->tk = LEX_R_FUNCTION;
	else if (strcmp(lex->tk_str->cstr, "true") == 0)      lex->tk = LEX_R_TRUE;
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

static var_t* json_parse_factor(vm_t* vm, lex_t *l) {
	if (l->tk==LEX_R_TRUE) {
		lex_js_chkread(l, LEX_R_TRUE);
		return var_new_int(vm, 1);
	}
	else if (l->tk==LEX_R_FALSE) {
		lex_js_chkread(l, LEX_R_FALSE);
		return var_new_int(vm, 0);
	}
	else if (l->tk==LEX_R_NULL) {
		lex_js_chkread(l, LEX_R_NULL);
		return var_new(vm);
	}
	else if (l->tk==LEX_R_UNDEFINED) {
		lex_js_chkread(l, LEX_R_UNDEFINED);
		return var_new(vm);
	}
	else if (l->tk==LEX_INT) {
		int i = atoi(l->tk_str->cstr);
		lex_js_chkread(l, LEX_INT);
		return var_new_int(vm, i);
	}
	else if (l->tk==LEX_FLOAT) {
		float f = atof(l->tk_str->cstr);
		lex_js_chkread(l, LEX_FLOAT);
		return var_new_float(vm, f);
	}
	else if (l->tk==LEX_STR) {
		str_t* s = str_new(l->tk_str->cstr);
		lex_js_chkread(l, LEX_STR);
		var_t* ret = var_new_str(vm, s->cstr);
		str_free(s);
		return ret;
	}
	else if(l->tk==LEX_R_FUNCTION) {
		lex_js_chkread(l, LEX_R_FUNCTION);
		//TODO
		//_err("Error: Can not parse json function item!\n");
		return var_new(vm);
	}
	else if (l->tk=='[') {
		/* JSON-style array */
		var_t* arr = var_new_array(vm);
		lex_js_chkread(l, '[');
		while (l->tk != ']') {
			var_t* v = json_parse_factor(vm, l);
			var_add(arr, "", v);
			if (l->tk != ']') 
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, ']');
		return arr;
	}
	else if (l->tk=='{') {
		lex_js_chkread(l, '{');
		var_t* obj = var_new_obj(vm, NULL, NULL);
		while(l->tk != '}') {
			str_t* id = str_new(l->tk_str->cstr);
			if(l->tk == LEX_STR)
				lex_js_chkread(l, LEX_STR);
			else
				lex_js_chkread(l, LEX_ID);

			lex_js_chkread(l, ':');
			var_t* v = json_parse_factor(vm, l);
			var_add(obj, id->cstr, v);
			str_free(id);
			if(l->tk != '}')
				lex_js_chkread(l, ',');
		}
		lex_js_chkread(l, '}');
		return obj;
	}
	return var_new(vm);
}

var_t* json_parse(vm_t* vm, const char* str) {
	lex_t lex;
	lex_init(&lex, str);
	lex_js_get_next_token(&lex);

	var_t* ret = json_parse_factor(vm, &lex);
	lex_release(&lex);
	return ret;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

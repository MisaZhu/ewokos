#include "mario/mario_lex.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

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
		if (lex->curr_ch=='x') {
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

#ifdef __cplusplus
}
#endif /* __cplusplus */


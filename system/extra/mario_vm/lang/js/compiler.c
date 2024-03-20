/**
	very tiny js script compiler.
 */

#include "mario.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* _mario_lang = "js";
/** Script Lex. -----------------------------*/

typedef enum {
	LEX_EQUAL = LEX_BASIC_END,
	LEX_TYPEEQUAL,
	LEX_NEQUAL,
	LEX_NTYPEEQUAL,
	LEX_LEQUAL,
	LEX_LSHIFT,
	LEX_LSHIFTEQUAL,
	LEX_GEQUAL,
	LEX_RSHIFT,
	LEX_RSHIFTUNSIGNED,
	LEX_RSHIFTEQUAL,
	LEX_PLUSEQUAL,
	LEX_MINUSEQUAL,
	LEX_MULTIEQUAL,
	LEX_DIVEQUAL,
	LEX_MODEQUAL,
	LEX_PLUSPLUS,
	LEX_MINUSMINUS,
	LEX_ANDEQUAL,
	LEX_ANDAND,
	LEX_OREQUAL,
	LEX_OROR,
	LEX_XOREQUAL,
	// reserved words
	LEX_R_IF,
	LEX_R_ELSE,
	LEX_R_DO,
	LEX_R_WHILE,
	LEX_R_FOR,
	LEX_R_BREAK,
	LEX_R_CONTINUE,
	LEX_R_STATIC,
	LEX_R_FUNCTION,
	LEX_R_AFUNCTION,
	LEX_R_CLASS,
	LEX_R_EXTENDS,
	LEX_R_RETURN,
	LEX_R_VAR,
	LEX_R_LET,
	LEX_R_CONST,
	LEX_R_TRUE,
	LEX_R_FALSE,
	LEX_R_NULL,
	LEX_R_UNDEFINED,
	LEX_R_NEW,
	LEX_R_TYPEOF,
	LEX_R_INCLUDE,
	LEX_R_THROW,
	LEX_R_TRY,
	LEX_R_CATCH,
	LEX_R_INSTANCEOF,
	LEX_R_LIST_END /* always the last entry */
} LEX_TYPES;

void lex_get_op_token(lex_t* lex) {
	if (lex->tk=='=' && lex->curr_ch=='=') { // ==
		lex->tk = LEX_EQUAL;
		lex_get_nextch(lex);
		if (lex->curr_ch=='=') { // ===
			lex->tk = LEX_TYPEEQUAL;
			lex_get_nextch(lex);
		}
	} else if (lex->tk=='!' && lex->curr_ch=='=') { // !=
		lex->tk = LEX_NEQUAL;
		lex_get_nextch(lex);
		if (lex->curr_ch=='=') { // !==
			lex->tk = LEX_NTYPEEQUAL;
			lex_get_nextch(lex);
		}
	} else if (lex->tk=='<' && lex->curr_ch=='=') {
		lex->tk = LEX_LEQUAL;
		lex_get_nextch(lex);
	} else if (lex->tk=='<' && lex->curr_ch=='<') {
		lex->tk = LEX_LSHIFT;
		lex_get_nextch(lex);
		if (lex->curr_ch=='=') { // <<=
			lex->tk = LEX_LSHIFTEQUAL;
			lex_get_nextch(lex);
		}
	} else if (lex->tk=='>' && lex->curr_ch=='=') {
		lex->tk = LEX_GEQUAL;
		lex_get_nextch(lex);
	} else if (lex->tk=='>' && lex->curr_ch=='>') {
		lex->tk = LEX_RSHIFT;
		lex_get_nextch(lex);
		if (lex->curr_ch=='=') { // >>=
			lex->tk = LEX_RSHIFTEQUAL;
			lex_get_nextch(lex);
		} else if (lex->curr_ch=='>') { // >>>
			lex->tk = LEX_RSHIFTUNSIGNED;
			lex_get_nextch(lex);
		}
	}  else if (lex->tk=='+' && lex->curr_ch=='=') {
		lex->tk = LEX_PLUSEQUAL;
		lex_get_nextch(lex);
	}  else if (lex->tk=='-' && lex->curr_ch=='=') {
		lex->tk = LEX_MINUSEQUAL;
		lex_get_nextch(lex);
	}  else if (lex->tk=='*' && lex->curr_ch=='=') {
		lex->tk = LEX_MULTIEQUAL;
		lex_get_nextch(lex);
	}  else if (lex->tk=='/' && lex->curr_ch=='=') {
		lex->tk = LEX_DIVEQUAL;
		lex_get_nextch(lex);
	}  else if (lex->tk=='%' && lex->curr_ch=='=') {
		lex->tk = LEX_MODEQUAL;
		lex_get_nextch(lex);
	}  else if (lex->tk=='+' && lex->curr_ch=='+') {
		lex->tk = LEX_PLUSPLUS;
		lex_get_nextch(lex);
	}  else if (lex->tk=='-' && lex->curr_ch=='-') {
		lex->tk = LEX_MINUSMINUS;
		lex_get_nextch(lex);
	} else if (lex->tk=='&' && lex->curr_ch=='=') {
		lex->tk = LEX_ANDEQUAL;
		lex_get_nextch(lex);
	} else if (lex->tk=='&' && lex->curr_ch=='&') {
		lex->tk = LEX_ANDAND;
		lex_get_nextch(lex);
	} else if (lex->tk=='|' && lex->curr_ch=='=') {
		lex->tk = LEX_OREQUAL;
		lex_get_nextch(lex);
	} else if (lex->tk=='|' && lex->curr_ch=='|') {
		lex->tk = LEX_OROR;
		lex_get_nextch(lex);
	} else if (lex->tk=='^' && lex->curr_ch=='=') {
		lex->tk = LEX_XOREQUAL;
		lex_get_nextch(lex);
	} else if (lex->tk=='=' && lex->curr_ch=='>') {
		lex->tk = LEX_R_AFUNCTION;
		lex_get_nextch(lex);
	}
}

void lex_get_js_str(lex_t* lex) {
	// js style strings 
	lex_get_nextch(lex);
	while (lex->curr_ch && lex->curr_ch!='\'') {
		if (lex->curr_ch == '\\') {
			lex_get_nextch(lex);
			switch (lex->curr_ch) {
				case 'n' : mstr_add(lex->tk_str, '\n'); break;
				case 'a' : mstr_add(lex->tk_str, '\a'); break;
				case 'r' : mstr_add(lex->tk_str, '\r'); break;
				case 't' : mstr_add(lex->tk_str, '\t'); break;
				case '\'' : mstr_add(lex->tk_str, '\''); break;
				case '\\' : mstr_add(lex->tk_str, '\\'); break;
				case 'x' : { // hex digits
										 char buf[3] = "??";
										 lex_get_nextch(lex);
										 buf[0] = lex->curr_ch;
										 lex_get_nextch(lex);
										 buf[1] = lex->curr_ch;
										 mstr_add(lex->tk_str, (char)strtol(buf,0,16));
									 } break;
				default: if (lex->curr_ch>='0' && lex->curr_ch<='7') {
									 // octal digits
									 char buf[4] = "???";
									 buf[0] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[1] = lex->curr_ch;
									 lex_get_nextch(lex);
									 buf[2] = lex->curr_ch;
									 mstr_add(lex->tk_str, (char)strtol(buf,0,8));
								 } else
									 mstr_add(lex->tk_str, lex->curr_ch);
			}
		} else {
			mstr_add(lex->tk_str, lex->curr_ch);
		}
		lex_get_nextch(lex);
	}
	lex_get_nextch(lex);
	lex->tk = LEX_STR;
}

void lex_get_reserved_word(lex_t *lex) {
	if (strcmp(lex->tk_str->cstr, "if") == 0)        lex->tk = LEX_R_IF;
	else if (strcmp(lex->tk_str->cstr, "else") == 0)      lex->tk = LEX_R_ELSE;
	else if (strcmp(lex->tk_str->cstr, "do") == 0)        lex->tk = LEX_R_DO;
	else if (strcmp(lex->tk_str->cstr, "while") == 0)    lex->tk = LEX_R_WHILE;
	else if (strcmp(lex->tk_str->cstr, "include") == 0)  lex->tk = LEX_R_INCLUDE;
	else if (strcmp(lex->tk_str->cstr, "for") == 0)     lex->tk = LEX_R_FOR;
	else if (strcmp(lex->tk_str->cstr, "break") == 0)    lex->tk = LEX_R_BREAK;
	else if (strcmp(lex->tk_str->cstr, "continue") == 0)  lex->tk = LEX_R_CONTINUE;
	else if (strcmp(lex->tk_str->cstr, "static") == 0)  lex->tk = LEX_R_STATIC;
	else if (strcmp(lex->tk_str->cstr, "function") == 0)  lex->tk = LEX_R_FUNCTION;
	else if (strcmp(lex->tk_str->cstr, "class") ==0) 		 lex->tk = LEX_R_CLASS;
	else if (strcmp(lex->tk_str->cstr, "extends") == 0) 	 lex->tk = LEX_R_EXTENDS;
	else if (strcmp(lex->tk_str->cstr, "return") == 0)   lex->tk = LEX_R_RETURN;
	else if (strcmp(lex->tk_str->cstr, "var")  == 0)      lex->tk = LEX_R_VAR;
	else if (strcmp(lex->tk_str->cstr, "let")  == 0)      lex->tk = LEX_R_LET;
	else if (strcmp(lex->tk_str->cstr, "const") == 0)     lex->tk = LEX_R_CONST;
	else if (strcmp(lex->tk_str->cstr, "true") == 0)      lex->tk = LEX_R_TRUE;
	else if (strcmp(lex->tk_str->cstr, "false") == 0)     lex->tk = LEX_R_FALSE;
	else if (strcmp(lex->tk_str->cstr, "null") == 0)      lex->tk = LEX_R_NULL;
	else if (strcmp(lex->tk_str->cstr, "undefined") == 0) lex->tk = LEX_R_UNDEFINED;
	else if (strcmp(lex->tk_str->cstr, "new") == 0)       lex->tk = LEX_R_NEW;
	else if (strcmp(lex->tk_str->cstr, "typeof") == 0)       lex->tk = LEX_R_TYPEOF;
	else if (strcmp(lex->tk_str->cstr, "throw") == 0)     lex->tk = LEX_R_THROW;
	else if (strcmp(lex->tk_str->cstr, "try") == 0)    	 lex->tk = LEX_R_TRY;
	else if (strcmp(lex->tk_str->cstr, "catch") == 0)     lex->tk = LEX_R_CATCH;
	else if (strcmp(lex->tk_str->cstr, "instanceof") == 0)     lex->tk = LEX_R_INSTANCEOF;
}

void lex_get_next_token(lex_t* lex) {
	lex->tk = LEX_EOF;
	mstr_reset(lex->tk_str);

	lex_skip_whitespace(lex);
	//lex_skip_space(lex);
	if(lex_skip_comments_line(lex, "//")) {
		lex_get_next_token(lex);
		return;
	}
	if(lex_skip_comments_block(lex, "/*", "*/")) {
		lex_get_next_token(lex);
		return;
	}

	lex_token_start(lex);
	lex_get_basic_token(lex);

	if (lex->tk == LEX_ID) { //  IDs
		lex_get_reserved_word(lex);
	} 
	else if(lex->tk == LEX_EOF) {
		if (lex->curr_ch=='\'') {
			// js style strings 
			lex_get_js_str(lex);
		} 
		else {
			lex_get_char_token(lex);
			lex_get_op_token(lex);
		}
	}

	lex_token_end(lex);
}

#ifdef MARIO_DEBUG

const char* lex_get_token_str(int token) {
	if (token>32 && token<128) {
		static char buf[4] = "' '";
		buf[1] = (char)token;
		return buf;
	}
	switch (token) {
		case LEX_EOF            : return "EOF";
		case LEX_ID             : return "ID";
		case LEX_INT            : return "INT";
		case LEX_FLOAT          : return "FLOAT";
		case LEX_STR            : return "STRING";
		case LEX_EQUAL          : return "==";
		case LEX_TYPEEQUAL      : return "===";
		case LEX_NEQUAL         : return "!=";
		case LEX_NTYPEEQUAL     : return "!==";
		case LEX_LEQUAL         : return "<=";
		case LEX_LSHIFT         : return "<<";
		case LEX_LSHIFTEQUAL    : return "<<=";
		case LEX_GEQUAL         : return ">=";
		case LEX_RSHIFT         : return ">>";
		case LEX_RSHIFTUNSIGNED : return ">>";
		case LEX_RSHIFTEQUAL    : return ">>=";
		case LEX_PLUSEQUAL      : return "+=";
		case LEX_MINUSEQUAL     : return "-=";
		case LEX_MULTIEQUAL     : return "*=";
		case LEX_DIVEQUAL 	    : return "/=";
		case LEX_MODEQUAL   	  : return "%=";
		case LEX_PLUSPLUS       : return "++";
		case LEX_MINUSMINUS     : return "--";
		case LEX_ANDEQUAL       : return "&=";
		case LEX_ANDAND         : return "&&";
		case LEX_OREQUAL        : return "|=";
		case LEX_OROR           : return "||";
		case LEX_XOREQUAL       : return "^=";
															// reserved words
		case LEX_R_IF           : return "if";
		case LEX_R_ELSE         : return "else";
		case LEX_R_DO           : return "do";
		case LEX_R_WHILE        : return "while";
		case LEX_R_FOR          : return "for";
		case LEX_R_BREAK        : return "break";
		case LEX_R_CONTINUE     : return "continue";
		case LEX_R_STATIC       : return "static";
		case LEX_R_FUNCTION     : return "function";
		case LEX_R_CLASS     		: return "class";
		case LEX_R_EXTENDS   		: return "extends";
		case LEX_R_RETURN       : return "return";
		case LEX_R_CONST        : return "CONST";
		case LEX_R_VAR          : return "var";
		case LEX_R_LET          : return "let";
		case LEX_R_TRUE         : return "true";
		case LEX_R_FALSE        : return "false";
		case LEX_R_NULL         : return "null";
		case LEX_R_UNDEFINED    : return "undefined";
		case LEX_R_NEW          : return "new";
		case LEX_R_INCLUDE      : return "include";
	}
	return "?[UNKNOW]";
}

#endif

void lex_get_pos_str(lex_t* l, int pos, mstr_t* ret) {
	int line = 1;
	int col;

	lex_get_pos(l, &line, &col, pos);
	mstr_append(ret, "(line: ");
	mstr_append(ret, mstr_from_int(line, 10));
	mstr_append(ret, ", col: ");
	mstr_append(ret, mstr_from_int(col, 10));
	mstr_append(ret, ")");
}

bool lex_chkread(lex_t* lex, uint32_t expected_tk);
bool lex_skip_empty(lex_t* l) {
	if(l->tk == '\n') {//skip empty lines.
		while (l->tk=='\n') 
			if(!lex_chkread(l, '\n')) return false;
	}
	return true;
}

bool lex_chkread(lex_t* lex, uint32_t expected_tk) { //check read with empty line.
	if (lex->tk != expected_tk) {
#ifdef MARIO_DEBUG
		mario_debug("Got ");
		mario_debug(lex_get_token_str(lex->tk));
		mario_debug(" expected ");
		mario_debug(lex_get_token_str(expected_tk));
#endif
		mstr_t* s = mstr_new("");
		lex_get_pos_str(lex, -1, s);
		mario_debug(s->cstr);
		mstr_free(s);
		mario_debug("!\n");
		return false;
	}
	lex_get_next_token(lex);
	return true;
}

/** Compiler -----------------------------*/

bool statement(lex_t*, bytecode_t*);
bool factor(lex_t*, bytecode_t*, bool member);
bool base(lex_t*, bytecode_t*);

void gen_func_name(const char* name, int arg_num, mstr_t* full) {
	mstr_reset(full);
	mstr_cpy(full, name);
	if(arg_num > 0) {
		mstr_append(full, "$");
		mstr_append(full, mstr_from_int(arg_num, 10));
	}
}

int call_func(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, '(')) return -1;
	int arg_num = 0;
	while(true) {
		PC pc1 = bc->cindex;
		if(!base(l, bc))
			return -1;
		PC pc2 = bc->cindex;
		if(pc2 > pc1) //not empty, means valid arguemnt.
			arg_num++;

		if (l->tk!=')') {
			if(!lex_chkread(l, ',')) return -1;	
		}
		else
			break;
	}
	if(!lex_chkread(l, ')')) return -1;
	return arg_num;
}

bool stmt_loop_block(lex_t* l, bytecode_t* bc) {
	bool block = false;
	if(l->tk && l->tk=='{') {
		if(!lex_chkread(l, '{')) return false;
		block = true;
	}

	if(block) {
		lex_skip_empty(l);
		while (l->tk && l->tk!='}'){
			if(!statement(l, bc))
				return false;
		}
		lex_skip_empty(l);
		return lex_chkread(l, '}');
	}
	lex_skip_empty(l);
	return statement(l, bc);
}

bool stmt_block(lex_t* l, bytecode_t* bc, bool func) {
	bool doBlock = false;
	if(!lex_chkread(l, '{')) return false;

	if(!func) 
		doBlock = true;

	if(doBlock)
		bc_gen(bc, INSTR_BLOCK);

	while (l->tk && l->tk!='}'){
		if(!statement(l, bc))
			return false;
	}
	if(!lex_chkread(l, '}')) return false;

	if(doBlock) 
		bc_gen(bc, INSTR_BLOCK_END);
	return true;
}

bool factor_def_func(lex_t* l, bytecode_t* bc, mstr_t* name) {
	bool is_static = false;
	lex_skip_empty(l);

	if (l->tk == LEX_R_STATIC) {
		if(!lex_chkread(l, LEX_R_STATIC)) return false;
		is_static = true;
	}

	/* we can have functions without names */
	if (l->tk == LEX_ID) {
		mstr_cpy(name, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;
	}
	if(l->tk == LEX_ID) { //class get/set token
		if(strcmp(name->cstr, "get") == 0) {
			mstr_cpy(name, l->tk_str->cstr);
			if(!lex_chkread(l, LEX_ID)) return false;
			bc_gen(bc, INSTR_FUNC_GET);
		}
		if(strcmp(name->cstr, "set") == 0) {
			mstr_cpy(name, l->tk_str->cstr);
			if(!lex_chkread(l, LEX_ID)) return false;
			bc_gen(bc, INSTR_FUNC_SET);
		}
	}
	else {
		bc_gen(bc, is_static ? INSTR_FUNC_STC:INSTR_FUNC);
	}
	lex_skip_empty(l);
	//do arguments
	if(!lex_chkread(l, '(')) return false;
	while (l->tk!=')') {
		bc_gen_str(bc, INSTR_LOAD, l->tk_str->cstr);
		//bc_gen_str(bc, INSTR_VAR, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;
		if (l->tk!=')') {
			if(!lex_chkread(l, ',')) return false;
		}
	}
	if(!lex_chkread(l, ')')) return false;
	lex_skip_empty(l);
	PC pc = bc_reserve(bc);
	stmt_block(l, bc, true);
	opr_code_t op = bc->code_buf[bc->cindex - 1] >> 16;

	if(op != INSTR_RETURN && op != INSTR_RETURNV)
		bc_gen(bc, INSTR_RETURN);
	bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);
	return true;
}

bool factor_def_afunc(lex_t* l, bytecode_t* bc) {
	lex_skip_empty(l);
	PC pc = bc_reserve(bc);
	statement(l, bc);

	opr_code_t op = bc->code_buf[bc->cindex - 1] >> 16;

	if(op != INSTR_RETURN && op != INSTR_RETURNV)
		bc_gen(bc, INSTR_RETURN);
	bc_set_instr(bc, pc, INSTR_JMP, ILLEGAL_PC);
	return true;	
}

static bool lex_chkread_stmt_end(lex_t* l) {
	if(l->tk == 0)
		return true;

	if(l->tk == ';')
		return lex_chkread(l, ';');
	else if(l->tk == '\n')
		return lex_chkread(l, '\n');
	return false;
	//return lex_chkread(l, ';');
}

bool factor_def_class(lex_t* l, bytecode_t* bc) {
	// actually parse a class...
	if(!lex_chkread(l, LEX_R_CLASS)) return false;
	mstr_t* name = mstr_new("");

	lex_skip_empty(l);
	/* we can have classes without names */
	if (l->tk==LEX_ID) {
		mstr_cpy(name, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;
	}
	bc_gen_str(bc, INSTR_CLASS, name->cstr);

	lex_skip_empty(l);
	/*read extends*/
	if (l->tk==LEX_R_EXTENDS) {
		if(!lex_chkread(l, LEX_R_EXTENDS)) return false;
		lex_skip_empty(l);
		mstr_cpy(name, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;
		bc_gen_str(bc, INSTR_EXTENDS, name->cstr);
	}

	lex_skip_empty(l);
	if(!lex_chkread(l, '{')) return false;
	lex_skip_empty(l);
	while (l->tk!='}') {
		if(l->tk == LEX_ID && l->next_ch == '=') {
			bc_gen_str(bc, INSTR_LOAD, l->tk_str->cstr);
			if(!lex_chkread(l, LEX_ID)) return false;
			if(!lex_chkread(l, '=')) return false;
			if(!base(l, bc)) return false;
			bc_gen(bc, INSTR_ASIGN);
			bc_gen(bc, INSTR_POP);
			lex_chkread_stmt_end(l);
			lex_skip_empty(l);
		}
		else {
			if(!factor_def_func(l, bc, name))
				return false;
			lex_skip_empty(l);
			bc_gen_str(bc, INSTR_MEMBERN, name->cstr);
		}
	}
	if(!lex_chkread(l, '}')) return false;
	bc_gen(bc, INSTR_CLASS_END);

	mstr_free(name);
	return true;
}

bool factor_new(lex_t*l, bytecode_t* bc) {
	// new -> create a new object
	if(!lex_chkread(l, LEX_R_NEW)) return false;
	mstr_t* class_name = mstr_new("");
	mstr_cpy(class_name, l->tk_str->cstr);

	if(!lex_chkread(l, LEX_ID)) return false;
	if (l->tk == '(') {
		int arg_num = call_func(l, bc);
		mstr_t* s = mstr_new("");
		gen_func_name(class_name->cstr, arg_num, s);
		bc_gen_str(bc, INSTR_NEW, s->cstr);
		mstr_free(s);
	}
	mstr_free(class_name);
	return true;
}

bool factor_json(lex_t*l, bytecode_t* bc) {
	if(!lex_chkread(l, '{')) return false;
	bc_gen(bc, INSTR_OBJ);
	while (l->tk != '}') {
		lex_skip_empty(l);
		mstr_t* id = mstr_new(l->tk_str->cstr);
		// we only allow strings or IDs on the left hand side of an initialisation
		if (l->tk==LEX_STR) {
			if(!lex_chkread(l, LEX_STR)) return false;
		}
		else {
			if(!lex_chkread(l, LEX_ID)) return false;
		}	

		if(!lex_chkread(l, ':')) return false;
		if(!base(l, bc)) return false;
		lex_skip_empty(l);
		bc_gen_str(bc, INSTR_MEMBERN, id->cstr);
		// no need to clean here, as it will definitely be used
		if (l->tk != '}') {
			if(!lex_chkread(l, ',')) return false;
		}
		mstr_free(id);
	}
	bc_gen(bc, INSTR_OBJ_END);
	return lex_chkread(l, '}');
}

bool factor_array(lex_t*l, bytecode_t* bc) {
	if(!lex_chkread(l, '[')) return false;
	bc_gen(bc, INSTR_ARRAY);
	while (l->tk != ']') {
		base(l, bc);
		bc_gen(bc, INSTR_MEMBER);
		if (l->tk != ']') {
			if(!lex_chkread(l, ',')) return false;
		}
	}
	if(!lex_chkread(l, ']')) return false;
	bc_gen(bc, INSTR_ARRAY_END);
	return true;
}

bool factor_call_func(lex_t* l, bytecode_t* bc, mstr_t* name, bool member) {
	mstr_t* s = mstr_new("");

	int arg_num = call_func(l, bc);
	gen_func_name(name->cstr, arg_num, s);
	bc_gen_str(bc, member ? INSTR_CALLO:INSTR_CALL, s->cstr);	
	mstr_free(s);
	return true;
}

bool factor_array_access(lex_t* l, bytecode_t* bc, mstr_t* name, bool member) {
	bc_gen_str(bc, member ? INSTR_GET:INSTR_LOAD, name->cstr);

	if(!lex_chkread(l, '[')) return false;
	if(!base(l, bc)) return false;
	if(!lex_chkread(l, ']')) return false;
	bc_gen(bc, INSTR_ARRAY_AT);
	return true;
}

bool factor(lex_t* l, bytecode_t* bc, bool member) {
	if (l->tk=='(') {
		PC pc = bc_reserve(bc);
		if(!lex_chkread(l, '(')) return false;
		if(!base(l, bc)) return false;
		lex_skip_empty(l);

		while(l->tk == ',') {
			if(!lex_chkread(l, ',')) return false;
			if(!base(l, bc)) return false;;
			lex_skip_empty(l);
		}
		if(!lex_chkread(l, ')')) return false;

		if(l->tk == LEX_R_AFUNCTION) {
			if(!lex_chkread(l, LEX_R_AFUNCTION)) return false;
			bc_set_instr(bc, pc, INSTR_FUNC, 0);
			factor_def_afunc(l, bc);
		}
		else {
			bc_remove_instr(bc, pc, 1);
		}
	}
	else if (l->tk==LEX_R_TRUE) {
		if(!lex_chkread(l, LEX_R_TRUE)) return false;
		bc_gen(bc, INSTR_TRUE);
	}
	else if (l->tk==LEX_R_FALSE) {
		if(!lex_chkread(l, LEX_R_FALSE)) return false;
		bc_gen(bc, INSTR_FALSE);
	}
	else if (l->tk==LEX_R_NULL) {
		if(!lex_chkread(l, LEX_R_NULL)) return false;
		bc_gen(bc, INSTR_NULL);
	}
	else if (l->tk==LEX_R_UNDEFINED) {
		if(!lex_chkread(l, LEX_R_UNDEFINED)) return false;
		bc_gen(bc, INSTR_UNDEF);
	}
	else if (l->tk==LEX_INT) {
		bc_gen_str(bc, INSTR_INT, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_INT)) return false;
	}
	else if (l->tk==LEX_FLOAT) {
		bc_gen_str(bc, INSTR_FLOAT, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_FLOAT)) return false;
	}
	else if (l->tk==LEX_STR) {
		bc_gen_str(bc, INSTR_STR, l->tk_str->cstr);
		if(!lex_chkread(l, LEX_STR)) return false;
	}
	else if(l->tk==LEX_R_FUNCTION) { //define function
		if(!lex_chkread(l, LEX_R_FUNCTION)) return false;
		mstr_t *fname = mstr_new("");
		factor_def_func(l, bc, fname);
		mstr_free(fname);
	}
	else if(l->tk==LEX_R_CLASS) { //define class
		factor_def_class(l, bc);
	} 
	else if (l->tk==LEX_R_NEW) { //new object
		factor_new(l, bc);
	}
	else if (l->tk=='{') { // JSON-style object definition
		factor_json(l, bc);
	}
	else if (l->tk=='[') { // JSON-style array 
		factor_array(l, bc);
	}
	else if(l->tk==LEX_ID) {
		mstr_t* name = mstr_new(l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;

		if (l->tk=='(') { // function call
			factor_call_func(l, bc, name, member);
		} 
		else if (l->tk == '[') { // array access
			factor_array_access(l, bc, name, member);
		}
		else {
			if(member) {
				bc_gen_str(bc, INSTR_GET, name->cstr);	
			}
			else if (l->tk == '.') {
				bc_gen_str(bc, INSTR_LOADO, name->cstr);	
			}
			else if (l->tk == LEX_R_AFUNCTION) {
				if(!lex_chkread(l, LEX_R_AFUNCTION)) return false;
				bc_gen(bc, INSTR_FUNC);
				bc_gen_str(bc, INSTR_LOAD, name->cstr);	
				factor_def_afunc(l, bc);
			}
			else {
				bc_gen_str(bc, INSTR_LOAD, name->cstr);	
			}
		}
		mstr_free(name);
	}

	if(l->tk == '.')  { // followed by member fetch
		if(!lex_chkread(l, '.')) return false;
		if(!factor(l, bc, true)) return false;;
	}
	return true;
}

bool unary(lex_t* l, bytecode_t* bc) {
	opr_code_t instr = INSTR_END;
	if (l->tk == '!') {
		if(!lex_chkread(l, '!')) return false;
		instr = INSTR_NOT;
	} else if(l->tk == LEX_R_TYPEOF) {
		if(!lex_chkread(l, LEX_R_TYPEOF)) return false;
		instr = INSTR_TYPEOF;
	}

	if(!factor(l, bc, false))
		return false;

	if(instr != INSTR_END) {
		bc_gen(bc, instr);
	}
	return true;	
}

bool term(lex_t* l, bytecode_t* bc) {
	if(!unary(l, bc))
		return false;

	while (l->tk=='*' || l->tk=='/' || l->tk=='%') {
		LEX_TYPES op = (LEX_TYPES)l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		if(!unary(l, bc)) return false;

		if(op == '*') {
			bc_gen(bc, INSTR_MULTI);
		}
		else if(op == '/') {
			bc_gen(bc, INSTR_DIV);
		}
		else {
			bc_gen(bc, INSTR_MOD);
		}
	}

	return true;	
}

bool expr(lex_t* l, bytecode_t* bc) {
	LEX_TYPES pre = (LEX_TYPES)l->tk;

	if (l->tk=='-') {
		if(!lex_chkread(l, '-')) return false;
	}
	else if(l->tk==LEX_PLUSPLUS) {
		if(!lex_chkread(l, LEX_PLUSPLUS)) return false;
	}
	else if(l->tk==LEX_MINUSMINUS) {
		if(!lex_chkread(l, LEX_MINUSMINUS)) return false;
	}

	if(!term(l, bc))
		return false;

	if (pre == '-') {
		bc_gen(bc, INSTR_NEG);
	}
	else if(pre==LEX_PLUSPLUS) {
		bc_gen(bc, INSTR_PPLUS_PRE);
	}
	else if(pre==LEX_MINUSMINUS) {
		bc_gen(bc, INSTR_MMINUS_PRE);
	}

	while (l->tk=='+' || l->tk=='-' ||
			l->tk==LEX_PLUSPLUS || l->tk==LEX_MINUSMINUS) {
		int op = l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		if (op==LEX_PLUSPLUS) {
			bc_gen(bc, INSTR_PPLUS);
		}
		else if(op==LEX_MINUSMINUS) {
			bc_gen(bc, INSTR_MMINUS);
		}
		else {
			if(!term(l, bc))
				return false;
			if(op== '+') {
				bc_gen(bc, INSTR_PLUS);
			}
			else if(op=='-') {
				bc_gen(bc, INSTR_MINUS);
			}
		}
	}

	return true;	
}

bool shift(lex_t* l, bytecode_t* bc) {
	if(!expr(l, bc))
		return false;

	if (l->tk==LEX_LSHIFT || l->tk==LEX_RSHIFT || l->tk==LEX_RSHIFTUNSIGNED) {
		int op = l->tk;
		if(!lex_chkread(l, op)) return false;
		if(!base(l, bc))
			return false;

		if (op==LEX_LSHIFT) {
			bc_gen(bc, INSTR_LSHIFT);
		}
		else if (op==LEX_RSHIFT) {
			bc_gen(bc, INSTR_RSHIFT);
		}
		else {
			bc_gen(bc, INSTR_URSHIFT);
		}
	}
	return true;	
}

bool condition(lex_t *l, bytecode_t* bc) {
	if(!shift(l, bc))
		return false;

	while (l->tk==LEX_EQUAL || l->tk==LEX_NEQUAL ||
			l->tk==LEX_TYPEEQUAL || l->tk==LEX_NTYPEEQUAL ||
			l->tk==LEX_LEQUAL || l->tk==LEX_GEQUAL ||
			l->tk==LEX_R_INSTANCEOF ||
			l->tk=='<' || l->tk=='>') {
		int op = l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		if(!shift(l, bc))
			return false;

		if(op == LEX_EQUAL) {
			bc_gen(bc, INSTR_EQ);
		}
		else if(op == LEX_NEQUAL) {
			bc_gen(bc, INSTR_NEQ);
		}
		else if(op == LEX_TYPEEQUAL) {
			bc_gen(bc, INSTR_TEQ);
		}
		else if(op == LEX_NTYPEEQUAL) {
			bc_gen(bc, INSTR_NTEQ);
		}
		else if(op == LEX_LEQUAL) {
			bc_gen(bc, INSTR_LEQ);
		}
		else if(op == LEX_GEQUAL) {
			bc_gen(bc, INSTR_GEQ);
		}
		else if(op == LEX_R_INSTANCEOF) {
			bc_gen(bc, INSTR_INSTOF);
		}
		else if(op == '>') {
			bc_gen(bc, INSTR_GRT);
		}
		else if(op == '<') {
			bc_gen(bc, INSTR_LES);
		}
	}

	return true;	
}

bool logic(lex_t* l, bytecode_t* bc) {
	if(!condition(l, bc))
		return false;

	while (l->tk=='&' || l->tk=='|' || l->tk=='^' || l->tk==LEX_ANDAND || l->tk==LEX_OROR) {
		int op = l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		if(!condition(l, bc))
			return false;

		if (op==LEX_ANDAND) {
			bc_gen(bc, INSTR_AAND);
		} 
		else if (op==LEX_OROR) {
			bc_gen(bc, INSTR_OOR);
		}
		else if (op=='|') {
			bc_gen(bc, INSTR_OR);
		}
		else if (op=='&') {
			bc_gen(bc, INSTR_AND);
		}
		else if (op=='^') {
			bc_gen(bc, INSTR_XOR);
		}
	}
	return true;	
}


bool ternary(lex_t *l, bytecode_t* bc) {
	if(!logic(l, bc))
		return false;

	if (l->tk=='?') {
		PC pc1 = bc_reserve(bc); //keep for jump
		if(!lex_chkread(l, '?')) return false;
		base(l, bc); //first choice
		PC pc2 = bc_reserve(bc); //keep for jump
		if(!lex_chkread(l, ':')) return false;
		bc_set_instr(bc, pc1, INSTR_NJMP, ILLEGAL_PC);
		base(l, bc); //second choice
		bc_set_instr(bc, pc2, INSTR_JMP, ILLEGAL_PC);
	} 
	return true;	
}

bool base(lex_t* l, bytecode_t* bc) {
	if(!ternary(l, bc))
		return false;

	if (l->tk=='=' || 
			l->tk==LEX_PLUSEQUAL ||
			l->tk==LEX_MULTIEQUAL ||
			l->tk==LEX_DIVEQUAL ||
			l->tk==LEX_MODEQUAL ||
			l->tk==LEX_MINUSEQUAL) {
		LEX_TYPES op = (LEX_TYPES)l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		base(l, bc);
		// sort out initialiser
		if (op == '=')  {
			bc_gen(bc, INSTR_ASIGN);
		}
		else if(op == LEX_PLUSEQUAL) {
			bc_gen(bc, INSTR_PLUSEQ);
		}
		else if(op == LEX_MINUSEQUAL) {
			bc_gen(bc, INSTR_MINUSEQ);
		}
		else if(op == LEX_MULTIEQUAL) {
			bc_gen(bc, INSTR_MULTIEQ);
		}
		else if(op == LEX_DIVEQUAL) {
			bc_gen(bc, INSTR_DIVEQ);
		}
		else if(op == LEX_MODEQUAL) {
			bc_gen(bc, INSTR_MODEQ);
		}
	}
	return true;
}

static bool is_stmt_end(int tk) {
	return (tk == ';' || tk == '\n' || tk == 0);
	//return (tk == ';');
}

bool stmt_var(lex_t* l, bytecode_t* bc) {
	opr_code_t op;

	if(l->tk == LEX_R_VAR) {
		if(!lex_chkread(l, LEX_R_VAR)) return false;
		op = INSTR_VAR;
	}
	else if(l->tk == LEX_R_LET) {
		if(!lex_chkread(l, LEX_R_LET)) return false;
		op = INSTR_LET;
	}
	else {
		if(!lex_chkread(l, LEX_R_CONST)) return false;
		op = INSTR_CONST;
	}

	while (!is_stmt_end(l->tk)) {
		mstr_t* vname = mstr_new(l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;
		bc_gen_str(bc, op, vname->cstr);
		// sort out initialiser
		if (l->tk == '=') {
			if(!lex_chkread(l, '=')) return false;
			bc_gen_str(bc, INSTR_LOAD, vname->cstr);
			if(!base(l, bc)) return false;
			bc_gen(bc, INSTR_ASIGN);
			bc_gen(bc, INSTR_POP);
		}
		if (!is_stmt_end(l->tk))
			if(!lex_chkread(l, ',')) return false;
		mstr_free(vname);
	}
	return lex_chkread_stmt_end(l);
}

bool stmt_if(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_IF)) return false;
	if(!lex_chkread(l, '(')) return false;
	if(!base(l, bc)) return false; //condition
	if(!lex_chkread(l, ')')) return false;
	PC pc = bc_reserve(bc);
	lex_skip_empty(l);
	if(!statement(l, bc)) return false;
	lex_skip_empty(l);

	if (l->tk == LEX_R_ELSE) {
		if(!lex_chkread(l, LEX_R_ELSE)) return false;
		PC pc2 = bc_reserve(bc);
		bc_set_instr(bc, pc, INSTR_NJMP, ILLEGAL_PC);
		lex_skip_empty(l);
		if(!statement(l, bc)) return false;
		bc_set_instr(bc, pc2, INSTR_JMP, ILLEGAL_PC);
	}
	else {
		bc_set_instr(bc, pc, INSTR_NJMP, ILLEGAL_PC);
	}
	return true;
}

bool stmt_while(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_WHILE)) return false;
	bc_gen(bc, INSTR_LOOP);
	PC pc = bc_reserve(bc); //to init, nil for while statement.
	PC pcc = bc_add_instr(bc, pc, INSTR_JMP, pc+2)-1; //jmp to loop (for continue anchor).
	PC pcb = bc_reserve(bc); //jump out of loop (for break anchor);

	if(!lex_chkread(l, '(')) return false;
	if(!base(l, bc)) return false; //condition
	if(!lex_chkread(l, ')')) return false;

	bc_add_instr(bc, pcb, INSTR_NJMPB, ILLEGAL_PC); //not jump back to break anchor;

	if(!stmt_loop_block(l, bc)) return false;

	bc_add_instr(bc, pcc, INSTR_JMPB, ILLEGAL_PC); //coninue anchor;
	pc = bc_gen(bc, INSTR_LOOP_END);
	bc_set_instr(bc, pcb, INSTR_JMP, pc-1); // end anchor;
	return true;
}

bool stmt_for(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_FOR)) return false;
	PC pc = bc_gen(bc, INSTR_LOOP);
	bc_add_instr(bc, pc, INSTR_JMP, pc+3); //jmp to init.
	PC pcc = bc_reserve(bc); //jump to condition (for continue anchor);
	PC pcb = bc_reserve(bc); //jump out of loop (for break anchor);

	if(!lex_chkread(l, '(')) return false;
	if(!statement(l, bc)) //init statement
		return false;

	lex_skip_empty(l);
	bc_set_instr(bc, pcc, INSTR_JMP, bc->cindex);
	if(!base(l, bc)) //condition
		return false; 
	if(!lex_chkread(l, ';')) return false;
	lex_skip_empty(l);
	bc_add_instr(bc, pcb, INSTR_NJMPB, ILLEGAL_PC); //jump out of loop if not condition.
	PC pcl = bc_reserve(bc); //jump to loop .skip the iterrator

	PC pci = bc->cindex;  //iterator anchor;
	if(!base(l, bc)) //iterator statement
		return false; 
	if(!lex_chkread(l, ')')) return false;
	bc_gen(bc, INSTR_POP); //pop the stack.

	bc_add_instr(bc, pcc, INSTR_JMPB, ILLEGAL_PC); //jump to coninue anchor;

	bc_set_instr(bc, pcl, INSTR_JMP, ILLEGAL_PC); // loop anchor;

	//if(!statement(l, bc)) return false; //loop statement
	if(!stmt_loop_block(l, bc)) return false;

	bc_add_instr(bc, pci, INSTR_JMPB, ILLEGAL_PC); //jump to iterator anchor;
	pc = bc_gen(bc, INSTR_LOOP_END);
	bc_set_instr(bc, pcb, INSTR_JMP, pc-1); // end anchor;
	return true;
}

bool stmt_break(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_BREAK)) return false;
	if(!lex_chkread_stmt_end(l)) return false;
	bc_gen(bc, INSTR_BREAK);
	return true;
}

bool stmt_continue(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_CONTINUE)) return false;
	if(!lex_chkread_stmt_end(l)) return false;
	bc_gen(bc, INSTR_CONTINUE);
	return true;
}

bool stmt_function(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_FUNCTION)) return false;
	mstr_t* fname = mstr_new("");
	factor_def_func(l, bc, fname);
	bc_gen_str(bc, INSTR_MEMBERN, fname->cstr);
	mstr_free(fname);
	return true;
}

bool stmt_include(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_INCLUDE)) return false;
	if(!base(l, bc)) return false;
	bc_gen(bc, INSTR_INCLUDE);
	return true;
}

bool stmt_return(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_RETURN)) return false;
	if (!is_stmt_end(l->tk)) {
		if(!base(l, bc)) return false;
		bc_gen(bc, INSTR_RETURNV);
	}
	else {
		bc_gen(bc, INSTR_RETURN);
	}
	return lex_chkread_stmt_end(l);
}

bool stmt_throw(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_THROW)) return false;
	if(!base(l, bc)) return false;
	if(!lex_chkread_stmt_end(l)) return false;
	bc_gen(bc, INSTR_THROW);
	return true;
}

bool stmt_try(lex_t* l, bytecode_t* bc) {
	if(!lex_chkread(l, LEX_R_TRY)) return false;
	PC pc = bc_gen(bc, INSTR_TRY);
	bc_add_instr(bc, pc, INSTR_JMP, pc+2);

	lex_skip_empty(l);
	PC pc_cache = bc_reserve(bc);
	if(!statement(l, bc)) return false;
	lex_skip_empty(l);
	PC pce = bc_reserve(bc); //jmp to finalize.

	bc_set_instr(bc, pc_cache, INSTR_JMP, ILLEGAL_PC);
	if(!lex_chkread(l, LEX_R_CATCH)) return false;

	lex_skip_empty(l);
	if(!lex_chkread(l, '(')) return false;
	bc_gen_str(bc, INSTR_CATCH, l->tk_str->cstr);
	if(!lex_chkread(l, LEX_ID)) return false;
	if(!lex_chkread(l, ')')) return false;
	lex_skip_empty(l);
	if(!statement(l, bc)) return false;

	pc = bc_gen(bc, INSTR_TRY_END) - 1;
	bc_set_instr(bc, pce, INSTR_JMP, pc); // end anchor;
	return true;
}

bool statement(lex_t* l, bytecode_t* bc) {
	bool pop = true;

	if(l->tk == '\n') {
		lex_skip_empty(l);
		pop = false;
	}
	else if (l->tk=='{') { /* A block of code */
		if(!stmt_block(l, bc, false)) return false;
		pop = false;
	}
	else if (l->tk == LEX_STR || 
			l->tk == LEX_INT || l->tk == LEX_FLOAT ||
			l->tk == '[' ||
			l->tk==LEX_ID ||
			l->tk==LEX_PLUSPLUS ||
			l->tk==LEX_MINUSMINUS ||
			l->tk=='-') {
		/* Execute a simple statement that only contains basic arithmetic... */
		if(!base(l, bc)) return false;
		if(is_stmt_end(l->tk))
			if(!lex_chkread_stmt_end(l)) return false;
	}
	else if (l->tk==LEX_R_VAR || l->tk == LEX_R_CONST || l->tk == LEX_R_LET) {
		if(!stmt_var(l, bc)) return false; 
		pop = false;
	}
	else if(l->tk == LEX_R_CLASS) {
		factor_def_class(l, bc);
	}
	else if(l->tk == LEX_R_FUNCTION) {
		if(!stmt_function(l, bc)) return false; 
		pop = false;
	}
	else if(l->tk == LEX_R_INCLUDE) {
		if(!stmt_include(l, bc)) return false; 
		pop = false;
	}
	else if (l->tk == LEX_R_RETURN) {
		if(!stmt_return(l, bc)) return false;
		pop = false;
	} 
	else if (l->tk == LEX_R_IF) {
		if(!stmt_if(l, bc)) return false;
		pop = false;
	}
	else if (l->tk == LEX_R_WHILE) {
		if(!stmt_while(l, bc)) return false;
		pop = false;
	}
	else if (l->tk==LEX_R_FOR) {
		if(!stmt_for(l, bc)) return false;
		pop = false;
	}
	else if(l->tk == LEX_R_BREAK) {
		if(!stmt_break(l, bc)) return false;
		pop = false;
	}
	else if(l->tk == LEX_R_CONTINUE) {
		if(!stmt_continue(l, bc)) return false;
		pop = false;
	}
	else if(l->tk == LEX_R_TRY) {
		if(!stmt_try(l, bc)) return false;
		pop = false;
	}
	else if(l->tk == LEX_R_THROW) {
		if(!stmt_throw(l, bc)) return false;
		pop = false;
	}
	else {
		mstr_t* s = mstr_new("Error: don't understand '");
		mstr_add(s, l->tk);
		mstr_append(s, l->tk_str->cstr);
		mstr_append(s, "', ");
		lex_get_pos_str(l, -1, s);
		mstr_append(s, "!\n");
		mario_debug(s->cstr);
		mstr_free(s);
		return false;
	}

	if(pop)
		bc_gen(bc, INSTR_POP);
	return true;
}

bool js_compile(bytecode_t *bc, const char* input) {
	lex_t lex;
	lex_init(&lex, input);
	lex_get_next_token(&lex);

	while(lex.tk) {
		if(!statement(&lex, bc)) {
			lex_release(&lex);
			return false;
		}
	}
	bc_gen(bc, INSTR_END);
	lex_release(&lex);
	return true;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


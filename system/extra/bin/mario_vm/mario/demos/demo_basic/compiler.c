/**
demo mario_vm compiler.
*/

#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif


const char* _mario_lang = "demo";
/** Script Lex. -----------------------------*/

typedef enum {
  // reserved words
  LEX_R_GOTO = LEX_BASIC_END,
  LEX_R_PRINT
} LEX_TYPES;

void lex_get_reserved(lex_t* lex) {
	if (strcmp(lex->tk_str->cstr, "print")  == 0) 
		lex->tk = LEX_R_PRINT;
	else if (strcmp(lex->tk_str->cstr, "goto")  == 0) 
		lex->tk = LEX_R_GOTO;
}

void lex_get_next_token(lex_t* lex) {
	lex->tk = LEX_EOF;
	str_reset(lex->tk_str);

	lex_skip_space(lex); //skip the space like ' ','\t','\r'. but keep '\n' for end of sentence.
	//skip comments.
	if(lex_skip_comments_line(lex, ";")) { //line comments
		lex_get_next_token(lex);
		return;
	}

	lex_token_start(lex); //
	//get basic tokens like LEX_INT, LEX_FLOAT, LEX_STR, LEX_ID, or LEX_EOF if got nothing.
	lex_get_basic_token(lex);

	if (lex->tk == LEX_ID) { //  IDs
		//try to replace LEX_ID token by reserved word.
		lex_get_reserved(lex);
	} 
	if(lex->tk == LEX_EOF) {
		//simplely set token with single char
		lex_get_char_token(lex);
	}
	lex_token_end(lex);
}

/**check current token with expected one, and read the next token*/
bool lex_chkread(lex_t* lex, uint32_t expected_tk) {
	if(lex->tk != expected_tk) 
		return false;

	lex_get_next_token(lex);
	return true;
}

/*function */
void gen_func_name(const char* name, int arg_num, str_t* full) {
	str_reset(full);
	str_cpy(full, name);
	if(arg_num > 0) {
		str_append(full, "$");
		str_append(full, str_from_int(arg_num, 10));
	}
}

bool base(lex_t* l, bytecode_t* bc);

int call_func(lex_t* l, bytecode_t* bc) {
	if(l->tk == '\n')
		return 0;

	int arg_num = 0;
	while(true) { // parameters.
		PC pc1 = bc->cindex;
		if(!base(l, bc)) 
			return -1;
		PC pc2 = bc->cindex;
		if(pc2 > pc1) //not empty, means valid arguemnt.
			arg_num++;

		if(l->tk != '\n') {
			if(!lex_chkread(l, ',')) return -1;	
		}
		else
			break;
	}
	return arg_num;
}

void factor_func(lex_t* l, bytecode_t* bc, const char* name) {
	int arg_num = call_func(l, bc);
	str_t* fname = str_new("");
	gen_func_name(name, arg_num, fname);
	bc_gen_str(bc, INSTR_CALL, fname->cstr);	
	str_free(fname);
}

bool factor(lex_t* l, bytecode_t* bc) {
	if (l->tk=='(') {
		if(!lex_chkread(l, '(')) return false;
		if(!base(l, bc)) return false;
		if(!lex_chkread(l, ')')) return false;
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
	else if(l->tk==LEX_ID) {
		str_t* name = str_new(l->tk_str->cstr);
		if(!lex_chkread(l, LEX_ID)) return false;

		bc_gen_str(bc, INSTR_LOAD, name->cstr);
		str_free(name);
	}

	return true;
}

bool base(lex_t* l, bytecode_t* bc) {
	if(!factor(l, bc))
		return false;

	if (l->tk=='=') {
		LEX_TYPES op = l->tk;
		if(!lex_chkread(l, l->tk)) return false;
		base(l, bc);
		// sort out initialiser
		if (op == '=')  {
			bc_gen(bc, INSTR_ASIGN);
		}
	}
	return true;
}

typedef struct {
	uint32_t line;
	PC pc;
} line_t;

m_array_t _lines;
m_array_t _gotos;

void add_line(m_array_t* lines, uint32_t ln, PC pc) {
	line_t* l = (line_t*)_malloc(sizeof(line_t));
	l->line = ln;
	l->pc = pc;
	array_add(lines, l);
}

PC get_line_pc(uint32_t ln) {
	int32_t i;
	for(i=0; i<_lines.size; ++i) {
		line_t* l = (line_t*)array_get(&_lines, i);
		if(l->line == ln)
			return l->pc;
	}
	return 0;
}

bool statement(lex_t* l, bytecode_t* bc) {
	bool pop = true;
	if(l->tk == '\n') {
		return lex_chkread(l, '\n');
	}

	//read line num
	int32_t ln = str_to_int(l->tk_str->cstr);
	if(!lex_chkread(l, LEX_INT)) return false;
	add_line(&_lines, ln, bc->cindex);

	if (l->tk == LEX_ID) {
		if(!base(l, bc))
			return false;
		if(!lex_chkread(l, '\n')) return false;
	}
	else if (l->tk == LEX_R_GOTO) {
		pop = false;
		if(!lex_chkread(l, LEX_R_GOTO)) return false;
		if(l->tk != LEX_INT) return false;
		int32_t ln_to = str_to_int(l->tk_str->cstr);
		if(!lex_chkread(l, LEX_INT)) return false;
		//keep the goto instruction to _gotos array, 
		//coz we have to calculate the jmp/jmpb offset after finished the compiling. 
		PC pc = bc_reserve(bc);
		add_line(&_gotos, ln_to, pc);
	}
	else if (l->tk == LEX_R_PRINT) {
		if(!lex_chkread(l, LEX_R_PRINT)) return false;
		factor_func(l, bc, "print");
		if(!lex_chkread(l, '\n')) return false;
	}
	else {
			mario_debug("Error: don't understand '");
			mario_debug(l->tk_str->cstr);
			mario_debug("'!\n");
			return false;
	}

	if(pop)
		bc_gen(bc, INSTR_POP);
	return true;
}

void handle_gotos(bytecode_t* bc) {
	int32_t i;
	for(i=0; i<_gotos.size; ++i) {
		line_t* l = (line_t*)array_get(&_gotos, i);
		PC pc = get_line_pc(l->line);
		opr_code_t op = pc > l->pc ? INSTR_JMP : INSTR_JMPB;
		bc_set_instr(bc, l->pc, op, pc); 
	}
}

bool compile(bytecode_t *bc, const char* input) {
	bool ret = true;
	array_init(&_lines);
	array_init(&_gotos);

	lex_t lex;
	lex_init(&lex, input);
	lex_get_next_token(&lex);

	while(lex.tk) {
		if(!statement(&lex, bc)) {
			lex_release(&lex);
			ret = false;
			break;
		}
	}
	bc_gen(bc, INSTR_END);
	lex_release(&lex);

	handle_gotos(bc);

	array_clean(&_gotos, NULL);
	array_clean(&_lines, NULL);
	return ret;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


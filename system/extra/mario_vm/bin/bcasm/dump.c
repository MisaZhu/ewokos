#include "mario.h"
#include <stdio.h>
#include <unistd.h>

const char* inmstr_str(opr_code_t ins) {
	switch(ins) {
		case  INSTR_NIL					: return "NIL";
		case  INSTR_END					: return "END";
		case  INSTR_OBJ					: return "OBJ";
		case  INSTR_OBJ_END			: return "OBJE";
		case  INSTR_MEMBER			: return "MEMBER";
		case  INSTR_MEMBERN			: return "MEMBERN";
		case  INSTR_POP					: return "POP";
		case  INSTR_VAR					: return "VAR";
		case  INSTR_LET					: return "LET";
		case  INSTR_CONST				: return "CONST";
		case  INSTR_INT					: return "INT";
		case  INSTR_INT_S				: return "INTS";
		case  INSTR_FLOAT				: return "FLOAT";
		case  INSTR_STR					: return "STR";
		case  INSTR_ARRAY_AT		: return "ARRAT";
		case  INSTR_ARRAY				: return "ARR";
		case  INSTR_ARRAY_END		: return "ARRE";
		case  INSTR_LOAD				: return "LOAD";
		case  INSTR_LOADO				: return "LOADO";
		case  INSTR_STORE				: return "STORE";
		case  INSTR_JMP					: return "JMP";
		case  INSTR_NJMP				: return "NJMP";
		case  INSTR_JMPB				: return "JMPB";
		case  INSTR_NJMPB				: return "NJMPB";
		case  INSTR_FUNC				: return "FUNC";
		case  INSTR_FUNC_STC		: return "FUNC_STC";
		case  INSTR_FUNC_GET		: return "FUNCGET";
		case  INSTR_FUNC_SET		: return "FUNCSET";
		case  INSTR_CLASS				: return "CLASS";
		case  INSTR_CLASS_END		: return "CLASSE";
		case  INSTR_EXTENDS			: return "EXTENDS";
		case  INSTR_CALL				: return "CALL";
		case  INSTR_CALLO				: return "CALLO";
		case  INSTR_NOT					: return "NOT";
		case  INSTR_MULTI				: return "MULTI";
		case  INSTR_DIV					: return "DIV";
		case  INSTR_MOD					: return "MOD";
		case  INSTR_PLUS				: return "PLUS";
		case  INSTR_MINUS				: return "MINUS";
		case  INSTR_NEG					: return "NEG";
		case  INSTR_PPLUS				: return "PPLUS";
		case  INSTR_MMINUS			: return "MMINUS";
		case  INSTR_PPLUS_PRE		: return "PPLUSP";
		case  INSTR_MMINUS_PRE	: return "MMINUSP";
		case  INSTR_LSHIFT			: return "LSHIFT";
		case  INSTR_RSHIFT			: return "RSHIFT";
		case  INSTR_URSHIFT			: return "URSHIFT";
		case  INSTR_EQ					: return "EQ";
		case  INSTR_NEQ					: return "NEQ";
		case  INSTR_LEQ					: return "LEQ";
		case  INSTR_GEQ					: return "GEQ";
		case  INSTR_GRT					: return "GRT";
		case  INSTR_LES					: return "LES";
		case  INSTR_PLUSEQ			: return "PLUSEQ";
		case  INSTR_MINUSEQ			: return "MINUSEQ";
		case  INSTR_MULTIEQ			: return "MULTIEQ";
		case  INSTR_DIVEQ				: return "DIVEQ";
		case  INSTR_MODEQ				: return "MODEQ";
		case  INSTR_AAND				: return "AAND";
		case  INSTR_OOR					: return "OOR";
		case  INSTR_OR					: return "OR";
		case  INSTR_XOR					: return "XOR";
		case  INSTR_AND					: return "AND";
		case  INSTR_ASIGN				: return "ASIGN";
		case  INSTR_BREAK				: return "BREAK";
		case  INSTR_CONTINUE		: return "CONTINUE";
		case  INSTR_RETURN			: return "RETURN";
		case  INSTR_RETURNV			: return "RETURNV";
		case  INSTR_TRUE				: return "TRUE";
		case  INSTR_FALSE				: return "FALSE";
		case  INSTR_NULL				: return "NULL";
		case  INSTR_UNDEF				: return "UNDEF";
		case  INSTR_NEW					: return "NEW";
		case  INSTR_GET					: return "GET";
		case  INSTR_BLOCK				: return "BLOCK";
		case  INSTR_BLOCK_END		: return "BLOCKE";
		case  INSTR_LOOP				: return "LOOP";
		case  INSTR_LOOP_END		: return "LOOPE";
		case  INSTR_TRY					: return "TRY";
		case  INSTR_TRY_END			: return "TRYE";
		case  INSTR_THROW				: return "THROW";
		case  INSTR_CATCH				: return "CATCH";
		case  INSTR_INSTOF			: return "INSTOF";
		case  INSTR_INCLUDE			: return "INCLUDE";
		default									: return "";
	}
}

PC bc_get_inmstr_str(bytecode_t* bc, PC i, mstr_t* ret) {
	PC ins = bc->code_buf[i];
	opr_code_t instr = OP(ins);
	uint32_t offset = ins & OFF_MASK;

	char s[128+1];
	mstr_reset(ret);

	if(offset == OFF_MASK) {
		snprintf(s, 128, "%08d | 0x%08X ; %s", i, ins, inmstr_str(instr));	
		mstr_append(ret, s);
	}
	else {
		if(instr == INSTR_JMP || 
				instr == INSTR_NJMP || 
				instr == INSTR_NJMPB ||
				instr == INSTR_JMPB ||
				instr == INSTR_INT_S) {
			snprintf(s, 128, "%08d | 0x%08X ; %s\t%d", i, ins, inmstr_str(instr), offset);	
			mstr_append(ret, s);
		}
		else {
			snprintf(s, 128, "%08d | 0x%08X ; %s\t\"", i, ins, inmstr_str(instr));	
			mstr_append(ret, s);
			mstr_append(ret, bc_getstr(bc, offset));
			mstr_add(ret, '"');
		}
	}
	
	if(instr == INSTR_INT) {
		ins = bc->code_buf[i+1];
		snprintf(s, 128, "\n%08d | 0x%08X ; %d", i+1, ins, ins);	
		mstr_append(ret, s);
		i++;
	}
	else if(instr == INSTR_FLOAT) {
		ins = bc->code_buf[i+1];
		float f;
		memcpy(&f, &ins, sizeof(PC));
		snprintf(s, 128, "\n%08d | 0x%08X ; %f", i+1, ins, f);	
		mstr_append(ret, s);
		i++;
	}	
	return i;
}

void bc_dump_out(bytecode_t* bc) {
    if(_out_func == NULL)
        return;

	PC i;
	char index[32];
	PC sz = bc->mstr_table.size;

	_out_func("mstr_index| value\n");
	_out_func("---------------------------------------\n");
	for(i=0; i<sz; ++i) {
		sprintf(index, "0x%06X | ", i);
		_out_func(index);
		_out_func((const char*)bc->mstr_table.items[i]);
		_out_func("\n");
	}
	_out_func("\npc_index | opr_code   ; instruction\n");
	_out_func("---------------------------------------\n");

	mstr_t* s = mstr_new("");

	i = 0;
	while(i < bc->cindex) {
		i = bc_get_inmstr_str(bc, i, s);
		_out_func(s->cstr);
		_out_func("\n");
		i++;
	}
	mstr_free(s);
	_out_func("---------------------------------------\n");
}

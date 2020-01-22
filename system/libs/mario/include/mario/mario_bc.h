/**
very tiny js engine in single file.
*/

#ifndef MARIO_BC
#define MARIO_BC

#include <types.h>
#include <marray.h>
#include <mstr.h>
#include <mstrx.h>

#ifdef __cplusplus /* __cplusplus */
extern "C" {
#endif

//bytecode
typedef uint32_t PC;
typedef uint16_t opr_code_t;
typedef struct st_bytecode {
	PC cindex;
	m_array_t str_table;
	PC *code_buf;
	uint32_t buf_size;
} bytecode_t;

/*
32bits bytecode:
+----------------------------------+
| 4bits  | 8bits    | 20bits       |
|----------------------------------|
| OPTION | OPR_CODE | OFFSET/VALUE |
+----------------------------------+
*/

#define ILLEGAL_PC 0xFFFFFFFF
#define INSTR_OPT_CACHE	 0x80000000
#define OFF_MASK 0x0FFFFF
#define INS(ins, off) (((((int32_t)ins) << 20) & 0xFFF00000) | ((off) & OFF_MASK))
#define OP(ins) (((ins) >>20) & 0xFF)
#define OFF(ins) ((ins) & OFF_MASK)

#define INSTR_NIL          0x000 // NIL           : Do nothing.

#define INSTR_VAR          0x001 // VAR x         : declare var x
#define INSTR_CONST        0x002 // CONST x       : declare const x
#define INSTR_LOAD         0x003 // LOAD x        : load and push x 
#define INSTR_LOADO        0x004 // LOAD obj x    : load and push obj x 
#define INSTR_STORE        0x005 // STORE x       : pop and store to x
#define INSTR_GET          0x006 // getfield
#define INSTR_ASIGN        0x007 // ASIGN         : =

#define INSTR_INT					 0x008 // INT int       : push int
#define INSTR_FLOAT        0x009 // FLOAT float   : push float 
#define INSTR_STR          0x00A // STR "str"     : push str
#define INSTR_ARRAY_AT     0x00B // ARRAT         : get array element at
#define INSTR_ARRAY        0x00C // ARRAY         : array start
#define INSTR_ARRAY_END    0x00D // ARRAY_END     : array end
#define INSTR_INT_S        0x00E // SHORT_INT int : push short int
#define INSTR_LET          0x00F // LET x         : declare let x

#define INSTR_FUNC         0x010 // FUNC x        : function definetion x
#define INSTR_FUNC_GET     0x011 // GET FUNC x    : class get function definetion x
#define INSTR_FUNC_SET     0x012 // SET FUNC x    : class set function definetion x
#define INSTR_CALL         0x013 // CALL x        : call function x and push res
#define INSTR_CALLO        0x014 // CALL obj.x    : call object member function x and push res
#define INSTR_CLASS        0x015 // class         
#define INSTR_CLASS_END    0x016 // class end			
#define INSTR_MEMBER       0x017 // member without name
#define INSTR_MEMBERN      0x018 // : member with name
#define INSTR_EXTENDS      0x019 // : class extends
#define INSTR_FUNC_STC     0x01A // ST FUNC x     : static function definetion x

#define INSTR_NOT          0x020 // NOT           : !
#define INSTR_MULTI        0x021 // MULTI         : *
#define INSTR_DIV          0x022 // DIV           : /
#define INSTR_MOD          0x023 // MOD           : %
#define INSTR_PLUS         0x024 // PLUS          : + 
#define INSTR_MINUS        0x025 // MINUS         : - 
#define INSTR_NEG          0x026 // NEG           : negate -
#define INSTR_PPLUS        0x027 // PPLUS         : x++
#define INSTR_MMINUS       0x028 // MMINUS        : x--
#define INSTR_PPLUS_PRE    0x029 // PPLUS         : ++x
#define INSTR_MMINUS_PRE   0x02A // MMINUS        : --x
#define INSTR_LSHIFT       0x02B // LSHIFT        : <<
#define INSTR_RSHIFT       0x02C // RSHIFT        : >>
#define INSTR_URSHIFT      0x02D // URSHIFT       : >>>

#define INSTR_EQ           0x030 // EQ            : ==
#define INSTR_NEQ          0x031 // NEQ           : !=
#define INSTR_LEQ          0x032 // LEQ           : <=
#define INSTR_GEQ          0x033 // GEQ           : >=
#define INSTR_GRT          0x034 // GRT           : >
#define INSTR_LES          0x035 // LES           : <

#define INSTR_PLUSEQ       0x036 // +=
#define INSTR_MINUSEQ      0x037 // -=	
#define INSTR_MULTIEQ      0x038 // *=
#define INSTR_DIVEQ        0x039 // /=
#define INSTR_MODEQ        0x03A // %=

#define INSTR_AAND         0x040 // AAND          : &&
#define INSTR_OOR          0x041 // OOR           : ||
#define INSTR_OR           0x042 // OR            : |
#define INSTR_XOR          0x043 // XOR           : ^
#define INSTR_AND          0x044 // AND           : &

#define INSTR_TEQ          0x046 // TEQ           : ===
#define INSTR_NTEQ         0x047 // NTEQ          : !==
#define INSTR_TYPEOF       0x048 // TYPEOF        : typeof

#define INSTR_BREAK        0x050 // break
#define INSTR_CONTINUE     0x051 // continue
#define INSTR_RETURN       0x052 // return none value
#define INSTR_RETURNV      0x053 // return with value

#define INSTR_NJMP         0x054 // NJMP x        : Condition not JMP offset x 
#define INSTR_JMPB         0x055 // JMP back x    : JMP back offset x  
#define INSTR_NJMPB        0x056 // NJMP back x   : Condition not JMP back offset x 
#define INSTR_JMP          0x057 // JMP x         : JMP offset x  

#define INSTR_TRUE         0x060 // true
#define INSTR_FALSE        0x061 // false
#define INSTR_NULL         0x062 // null
#define INSTR_UNDEF        0x063 // undefined

#define INSTR_NEW          0x070 // new
#define INSTR_CACHE        0x071 // CACHE index   : load cache at 'index' and push 

#define INSTR_POP          0x080 // pop and release

#define INSTR_OBJ          0x090 // object for JSON 
#define INSTR_OBJ_END      0x091 // object end for JSON 

#define INSTR_BLOCK        0x0A0 // block 
#define INSTR_BLOCK_END    0x0A1 // block end 
#define INSTR_LOOP         0x0A2 // loop
#define INSTR_LOOP_END     0x0A3 // loop end
#define INSTR_TRY          0x0A4 // try
#define INSTR_TRY_END      0x0A5 // try end

#define INSTR_THROW        0x0B0 // throw
#define INSTR_CATCH        0x0B1 // catch
#define INSTR_INSTOF       0x0B2 // instanceof

#define INSTR_INCLUDE      0x0C0 // include

#define INSTR_END          0x0FF //END : end of code.


PC bc_gen(bytecode_t* bc, opr_code_t instr);
PC bc_gen_str(bytecode_t* bc, opr_code_t instr, const char* s);
PC bc_gen_int(bytecode_t* bc, opr_code_t instr, int32_t i);
PC bc_gen_short(bytecode_t* bc, opr_code_t instr, int32_t i);
void bc_set_instr(bytecode_t* bc, PC anchor, opr_code_t op, PC target);
void bc_remove_instr(bytecode_t* bc, PC from, uint32_t num);
uint32_t bc_getstrindex(bytecode_t* bc, const char* str);
PC bc_add_instr(bytecode_t* bc, PC anchor, opr_code_t op, PC target);
PC bc_reserve(bytecode_t* bc);

#define bc_getstr(bc, i) (((i)>=(bc)->str_table.size) ? "" : (const char*)(bc)->str_table.items[(i)])

void bc_dump(bytecode_t* bc);
void bc_init(bytecode_t* bc);
void bc_release(bytecode_t* bc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

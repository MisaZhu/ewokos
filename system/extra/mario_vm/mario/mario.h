#ifndef MARIO_H
#define MARIO_H

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**====== platform porting functions.======*/
extern void* (*_malloc)(uint32_t size);
extern void  (*_free)(void* p);
extern void  (*_out_func)(const char*);

/**====== memory functions.======*/
extern void* _realloc(void* p, uint32_t old_size, uint32_t new_size);

/**====== debug functions.======*/
extern bool _m_debug;
void mario_debug(const char* s);

/**====== array functions. ======*/
#define STATIC_mstr_MAX 32
typedef void (*free_func_t)(void* p);

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

/**====== string functions. ======*/

typedef struct st_mstr {
	char* cstr;
	uint32_t max: 16;
	uint32_t len: 16;
} mstr_t;

void mstr_reset(mstr_t* str);
char* mstr_ncpy(mstr_t* str, const char* src, uint32_t l);
char* mstr_cpy(mstr_t* str, const char* src);
mstr_t* mstr_new(const char* s);
mstr_t* mstr_new_by_size(uint32_t sz);
char* mstr_append(mstr_t* str, const char* src);
char* mstr_add(mstr_t* str, char c);
char* mstr_add_int(mstr_t* str, int i, int base);
char* mstr_add_float(mstr_t* str, float f);
void mstr_free(mstr_t* str);
const char* mstr_from_int(int i, int base);
const char* mstr_from_float(float f);
const char* mstr_from_bool(bool b);
int mstr_to_int(const char* str);
float mstr_to_float(const char* str);
void mstr_split(const char* str, char c, m_array_t* array);
int mstr_to(const char* str, char c, mstr_t* res, bool skipspace);


/**======utf8 string functions =======*/

typedef struct st_utf8_reader {
	const char* str;
	uint32_t offset;
} utf8_reader_t;

typedef m_array_t utf8_t;

void utf8_reader_init(utf8_reader_t* reader, const char* s, uint32_t offset);
bool utf8_read(utf8_reader_t* reader, mstr_t* dst);

utf8_t* utf8_new(const char* s);
void utf8_free(utf8_t* utf8);
void utf8_append_raw(utf8_t* utf8, const char* s);
void utf8_append(utf8_t* utf8, const char* s);
uint32_t utf8_len(utf8_t* utf8);
mstr_t* utf8_at(utf8_t* utf8, uint32_t at);
void utf8_set(utf8_t* utf8, uint32_t at, const char* s);
void utf8_to_str(utf8_t* utf8, mstr_t* str);


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
	mstr_t* tk_str;
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

/**====== MARIO_BC ======*/

//bytecode
typedef uint32_t PC;
typedef uint16_t opr_code_t;
typedef struct st_bytecode {
	PC cindex;
	m_array_t mstr_table;
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

#define bc_getstr(bc, i) (((i)>=(bc)->mstr_table.size) ? "" : (const char*)(bc)->mstr_table.items[(i)])

void bc_init(bytecode_t* bc);
void bc_release(bytecode_t* bc);


/**====== mario_vm ======*/

extern const char* _mario_lang;

//script var
#define V_UNDEF  0
#define V_INT    1
#define V_FLOAT  2
#define V_STRING 3
#define V_OBJECT 4
#define V_BOOL   5
#define V_NULL   6

#define V_ST_FREE      0
#define V_ST_GC_FREE   1
#define V_ST_GC        2
#define V_ST_REF       3 

#define THIS "this"
#define PROTOTYPE "prototype"
#define SUPER "super"
#define CONSTRUCTOR "constructor"

struct st_vm;

typedef struct st_var {
	uint32_t magic: 8; //0 for var; 1 for node
	uint32_t type:10;
	uint32_t status: 4;
	uint32_t is_array:2;
	uint32_t is_func:2;
	uint32_t is_class:2;
	uint32_t gc_marking: 2;
	uint32_t gc_marked: 2;
	uint32_t refs;

	uint32_t size;  // size for bytes type of value;
	void* value;

	free_func_t free_func; //how to free value
	free_func_t on_destroy; //before destroyed.

	struct st_var* prev; //for var list
	struct st_var* next; //for var list
	m_array_t children;
	struct st_vm* vm;
} var_t;

typedef var_t* (*native_func_t)(struct st_vm *, var_t*, void*);

typedef struct st_func {
	native_func_t native;
	int8_t regular: 4;
	int8_t is_static: 4;
	PC pc;
	void *data;
	m_array_t args; //argument names
	var_t* owner;
} func_t;

//script node for var member children
typedef struct st_node {
	int16_t magic: 8; //1 for node
  int16_t be_const : 8;
	char* name;
	var_t* var;
} node_t;


#ifdef MARIO_THREAD
#include <pthread.h>

typedef struct st_isignal {
	var_t* obj;
	var_t* handle_func;
	mstr_t* handle_func_name;
	mstr_t* msg;
	struct st_isignal* next;
	struct st_isignal* prev;
} isignal_t;
#endif

#ifdef MARIO_CACHE
#define VAR_CACHE_MAX 32
#endif

#define VM_STACK_MAX    32

typedef struct st_vm {
	bytecode_t bc;
	bool (*compiler)(bytecode_t *bc, const char* input);

	m_array_t *scopes;
	void* stack[VM_STACK_MAX];
	int32_t stack_top;
	PC pc;

	bool terminated;
	var_t* root;

	m_array_t included;

	void (*on_init)(struct st_vm* vm);
	m_array_t init_natives;
	void (*on_close)(struct st_vm* vm);
	m_array_t close_natives;

	#ifdef MARIO_THREAD
	pthread_mutex_t thread_lock;
	isignal_t* isignal_head;
	isignal_t* isignal_tail;
	uint32_t isignal_num;
	bool interrupted;
	#endif

	#ifdef MARIO_CACHE
	var_t* var_cache[VAR_CACHE_MAX];
	uint32_t var_cache_used;
	#endif

	uint32_t this_strIndex;
	var_t* var_Object;
	var_t* var_String;
	var_t* var_Number;
	var_t* var_Array;
	var_t* var_true;
	var_t* var_false;
	var_t* var_null;

	//for gc
	bool is_doing_gc;
	uint32_t gc_buffer_size;
	uint32_t gc_free_buffer_size;
	var_t* gc_vars;
	var_t* gc_vars_tail;
	uint32_t gc_vars_num;
	var_t* free_vars;
	uint32_t free_vars_num;
} vm_t;

typedef mstr_t* (*load_m_func_t)(struct st_vm *, const char* jsname);
extern load_m_func_t _load_m_func;

node_t* node_new(vm_t* vm, const char* name, var_t* var);
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
void var_instance_from(var_t* var, var_t* src);
void var_from_prototype(var_t* var, var_t* proto);
void var_clean(var_t* var);

var_t* var_ref(var_t* var);
void var_unref(var_t* var);

//#define var_ref(var) ({ ++(var)->refs; var; })
//#define var_unref(var, del) ({ --(var)->refs; if((var)->refs <= 0 && (del)) var_free((var)); })

var_t* var_new(vm_t* vm);
var_t* var_new_block(vm_t* vm);
var_t* var_new_array(vm_t* vm);
var_t* var_new_int(vm_t* vm, int i);
var_t* var_new_null(vm_t* vm);
var_t* var_new_bool(vm_t* vm, bool b);
var_t* var_new_obj(vm_t* vm, void*p, free_func_t fr);
var_t* var_new_float(vm_t* vm, float i);
var_t* var_new_str(vm_t* vm, const char* s);
var_t* var_new_str2(vm_t* vm, const char* s, uint32_t len);
const char* var_get_str(var_t* var);
var_t* var_set_str(var_t* var, const char* v);
int var_get_int(var_t* var);
var_t* var_set_int(var_t* var, int v);
bool var_get_bool(var_t* var);
float var_get_float(var_t* var);
var_t* var_set_float(var_t* var, float v);
func_t* var_get_func(var_t* var);
var_t* var_get_prototype(var_t* var);
bool var_instanceof(var_t* var, var_t* proto);

void var_to_json_str(var_t*, mstr_t*, int);
void var_to_str(var_t*, mstr_t*);

void vm_push(vm_t* vm, var_t* var);
void vm_push_node(vm_t* vm, node_t* node);
vm_t* vm_new(bool (*compiler)(bytecode_t *bc, const char* input));
node_t* vm_load_node(vm_t* vm, const char* name, bool create);

void vm_init(vm_t* vm,
	void (*on_init)(struct st_vm* vm),
	void (*on_close)(struct st_vm* vm)
);

vm_t* vm_from(vm_t* vm);

bool vm_load(vm_t* vm, const char* s);
bool vm_load_run(vm_t* vm, const char* s);
bool vm_load_run_native(vm_t* vm, const char* s);
bool vm_run(vm_t* vm);
void vm_close(vm_t* vm);

var_t* vm_new_class(vm_t* vm, const char* cls);
var_t* new_obj(vm_t* vm, const char* cls_name, int arg_num);
node_t* vm_find(vm_t* vm, const char* name);
node_t* vm_find_in_class(var_t* var, const char* name);
node_t* vm_reg_var(vm_t* vm, var_t* cls, const char* name, var_t* var, bool be_const);
node_t* vm_reg_static(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
node_t* vm_reg_native(vm_t* vm, var_t* cls, const char* decl, native_func_t native, void* data);
void vm_mark_func_scopes(vm_t* vm, var_t* func);
void vm_reg_init(vm_t* vm, void (*func)(void*), void* data);
void vm_reg_close(vm_t* vm, void (*func)(void*), void* data);

node_t* find_member(var_t* obj, const char* name);
var_t* get_obj(var_t* obj, const char* name);
void* get_raw(var_t* obj, const char* name);
const char* get_str(var_t* obj, const char* name);
int get_int(var_t* obj, const char* name);
float get_float(var_t* obj, const char* name);
bool get_bool(var_t* obj, const char* name);
var_t* get_obj_member(var_t* obj, const char* name);
var_t* set_obj_member(var_t* obj, const char* name, var_t* var);

var_t* get_func_args(var_t* env);
uint32_t get_func_args_num(var_t* env);
var_t* get_func_arg(var_t* env, uint32_t index);
int get_func_arg_int(var_t* env, uint32_t index);
bool get_func_arg_bool(var_t* env, uint32_t index);
float get_func_arg_float(var_t* env, uint32_t index);
const char* get_func_arg_str(var_t* env, uint32_t index);
var_t* call_m_func(vm_t* vm, var_t* obj, var_t* func, var_t* args);
var_t* call_m_func_by_name(vm_t* vm, var_t* obj, const char* func_name, var_t* args);

bool interrupt(vm_t* vm, var_t* obj, var_t* func, const char* msg);
bool interrupt_by_name(vm_t* vm, var_t* obj, const char* func_name, const char* msg);

extern var_t* json_parse(vm_t* vm, const char* str);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

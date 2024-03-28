#include "native_math.h"

#include <math.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*=====math native functions=========*/

#define K_E                 exp(1.0)
#define K_PI                3.1415926535897932384626433832795

#define F_ABS(a)            ((a)>=0 ? (a) : (-(a)))
#define F_MIN(a,b)          ((a)>(b) ? (b) : (a))
#define F_MAX(a,b)          ((a)>(b) ? (a) : (b))
#define F_SGN(a)            ((a)>0 ? 1 : ((a)<0 ? -1 : 0 ))
#define F_RNG(a,min,max)    ((a)<(min) ? min : ((a)>(max) ? max : a ))
#define F_ROUND(a)          ((a)>0 ? (int) ((a)+0.5) : (int) ((a)-0.5) )

//Math.abs(x) - returns absolute of given value
var_t* native_math_abs(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "a");
	if(n == NULL || n->var == NULL)
		return NULL;

	if (n->var->type == V_INT) {
		return var_new_int(vm, (F_ABS(var_get_int(n->var))));
	}	
	else if (n->var->type == V_FLOAT) {
		return var_new_float(vm, (F_ABS(var_get_float(n->var))));
	}
	return NULL;
}

//Math.round(a) - returns nearest round of given value
var_t* native_math_round(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "a");
	if(n == NULL || n->var == NULL)
		return NULL;

	if (n->var->type == V_FLOAT) 
		return var_new_float(vm, (F_ROUND(var_get_float(n->var))));
	return var_new_int(vm, (F_ROUND(var_get_int(n->var))));
}

//Math.min(a,b) - returns minimum of two given values 
var_t* native_math_min(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "a");
	if(n == NULL) return NULL;
	var_t* varA = n->var;

	n = var_find(env, "b");
	if(n == NULL) return NULL;
	var_t* varB = n->var;

	if(varA == NULL || varB == NULL)
		return NULL;

	if (varA->type == V_INT && varB->type == V_INT)
		return var_new_int(vm, F_MIN(var_get_int(varA), var_get_int(varB)));
	return var_new_float(vm, F_MIN(var_get_float(varA), var_get_float(varB)));
}

//Math.max(a,b) - returns maximum of two given values  
var_t* native_math_max(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "a");
	if(n == NULL) return NULL;
	var_t* varA = n->var;

	n = var_find(env, "b");
	if(n == NULL) return NULL;
	var_t* varB = n->var;

	if(varA == NULL || varB == NULL)
		return NULL;

	if (varA->type == V_INT && varB->type == V_INT)
		return var_new_int(vm, F_MAX(var_get_int(varA), var_get_int(varB)));
	return var_new_float(vm, F_MAX(var_get_float(varA), var_get_float(varB)));
}

//Math.range(x,a,b) - returns value limited between two given values  
var_t* native_math_range(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "x");
	if(n == NULL) return NULL;
	var_t* varX = n->var;

	n = var_find(env, "a");
	if(n == NULL) return NULL;
	var_t* varA = n->var;

	n = var_find(env, "b");
	if(n == NULL) return NULL;
	var_t* varB = n->var;

	if(varX == NULL || varA == NULL || varB == NULL)
		return NULL;

	if (varX->type == V_INT)
		return var_new_int(vm, F_RNG(var_get_int(varX), var_get_int(varA), var_get_int(varB)));
	return var_new_float(vm, F_RNG(var_get_float(varX), var_get_float(varA), var_get_float(varB)));
}

//Math.sign(a) - returns sign of given value (-1==negative,0=zero,1=positive)
var_t* native_math_sign(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	node_t* n = var_find(env, "a");
	if(n == NULL) return NULL;
	var_t* var = n->var;

	if (var->type == V_INT) 
		return var_new_int(vm, F_SGN(var_get_int(var)));
	return var_new_float(vm, F_SGN(var_get_float(var)));
}

//Math.PI() - returns PI value
var_t* native_math_PI(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)env; (void)data;
	return var_new_float(vm, K_PI);
}

//Math.toDegrees(a) - returns degree value of a given angle in radians
var_t* native_math_toDegrees(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(f));
}

//Math.toRadians(a) - returns radians value of a given angle in degrees
var_t* native_math_toRadians(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (K_PI/180.0)*(f));
}

//Math.sin(a) - returns trig. sine of given angle in radians
var_t* native_math_sin(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(sin(f)));
}

//Math.asin(a) - returns trig. arcsine of given angle in radians
var_t* native_math_asin(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(asin(f)));
}

//Math.cos(a) - returns trig. cosine of given angle in radians
var_t* native_math_cos(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(cos(f)));
}

//Math.acos(a) - returns trig. arccosine of given angle in radians
var_t* native_math_acos(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(acos(f)));
}

//Math.tan(a) - returns trig. tangent of given angle in radians
var_t* native_math_tan(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(tan(f)));
}

//Math.atan(a) - returns trig. arctangent of given angle in radians
var_t* native_math_atan(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(atan(f)));
}

//Math.sinh(a) - returns trig. hyperbolic sine of given angle in radians
var_t* native_math_sinh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(sinh(f)));
}

//Math.asinh(a) - returns trig. hyperbolic arcsine of given angle in radians
var_t* native_math_asinh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(asinh(f)));
}

//Math.cosh(a) - returns trig. hyperbolic cosine of given angle in radians
var_t* native_math_cosh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(cosh(f)));
}

//Math.acosh(a) - returns trig. hyperbolic arccosine of given angle in radians
var_t* native_math_acosh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(acosh(f)));
}

//Math.tanh(a) - returns trig. hyperbolic tangent of given angle in radians
var_t* native_math_tanh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(tanh(f)));
}

//Math.atan(a) - returns trig. hyperbolic arctangent of given angle in radians
var_t* native_math_atanh(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(atanh(f)));
}

//Math.E() - returns E Neplero value
var_t* native_math_E(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)env; (void)data;
	return var_new_float(vm, K_E);
}

//Math.log(a) - returns natural logaritm (base E) of given value
var_t* native_math_log(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(log(f)));
}

//Math.log10(a) - returns logaritm(base 10) of given value
var_t* native_math_log10(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(log10(f)));
}

//Math.exp(a) - returns e raised to the power of a given number
var_t* native_math_exp(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, (180.0/K_PI)*(exp(f)));
}

//Math.pow(a,b) - returns the result of a number raised to a power (a)^(b)
var_t* native_math_pow(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float fa = get_float(env, "a");
	float fb = get_float(env, "b");
	return var_new_float(vm, pow(fa, fb));
}

//Math.sqr(a) - returns square of given value
var_t* native_math_sqr(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, f*f);
}

//Math.sqrt(a) - returns square root of given value
var_t* native_math_sqrt(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	float f = get_float(env, "a");
	return var_new_float(vm, sqrtf(f));
}

//Math.rand() - returns random double number
var_t* native_math_rand(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)env; (void)data;
	return var_new_float(vm, ((double)rand()/RAND_MAX));
}

//Math.randInt(min, max) - returns random int number
var_t* native_math_randInt(vm_t* vm, var_t* env, void *data) {
	(void)vm; (void)data;
	int min = get_int(env, "min");
	int max = get_int(env, "max");
	int val = min + (int)(rand()%(1+max-min));
	return var_new_int(vm, val);
}


#define CLS_MATH "Math"

void reg_native_math(vm_t* vm) {
	var_t* cls = vm_new_class(vm, CLS_MATH);
	vm_reg_static(vm, cls, "abs(a)", native_math_abs, NULL);
	vm_reg_static(vm, cls, "round(a)", native_math_round, NULL);
	vm_reg_static(vm, cls, "min(a,b)", native_math_min, NULL);
	vm_reg_static(vm, cls, "max(a,b)", native_math_max, NULL);
	vm_reg_static(vm, cls, "range(x,a,b)", native_math_range, NULL);
	vm_reg_static(vm, cls, "sign(a)", native_math_sign, NULL);

	vm_reg_static(vm, cls, "PI()", native_math_PI, NULL);
	vm_reg_static(vm, cls, "toDegrees(a)", native_math_toDegrees, NULL);
	vm_reg_static(vm, cls, "toRadians(a)", native_math_toRadians, NULL);
	vm_reg_static(vm, cls, "sin(a)", native_math_sin, NULL);
	vm_reg_static(vm, cls, "asin(a)", native_math_asin, NULL);
	vm_reg_static(vm, cls, "cos(a)", native_math_cos, NULL);
	vm_reg_static(vm, cls, "acos(a)", native_math_acos, NULL);
	vm_reg_static(vm, cls, "tan(a)", native_math_tan, NULL);
	vm_reg_static(vm, cls, "atan(a)", native_math_atan, NULL);
	vm_reg_static(vm, cls, "sinh(a)", native_math_sinh, NULL);
	vm_reg_static(vm, cls, "asinh(a)", native_math_asinh, NULL);
	vm_reg_static(vm, cls, "cosh(a)", native_math_cosh, NULL);
	vm_reg_static(vm, cls, "acosh(a)", native_math_acosh, NULL);
	vm_reg_static(vm, cls, "tanh(a)", native_math_tanh, NULL);
	vm_reg_static(vm, cls, "atanh(a)", native_math_atanh, NULL);

	vm_reg_static(vm, cls, "E()", native_math_E, NULL);
	vm_reg_static(vm, cls, "log(a)", native_math_log, NULL);
	vm_reg_static(vm, cls, "log10(a)", native_math_log10, NULL);
	vm_reg_static(vm, cls, "exp(a)", native_math_exp, NULL);
	vm_reg_static(vm, cls, "pow(a,b)", native_math_pow, NULL);

	vm_reg_static(vm, cls, "sqr(a)", native_math_sqr, NULL);
	vm_reg_static(vm, cls, "sqrt(a)", native_math_sqrt, NULL);    
	vm_reg_static(vm, cls, "rand()", native_math_rand, NULL);
	vm_reg_static(vm, cls, "randInt(min, max)", native_math_randInt, NULL); 
}

#ifdef __cplusplus
}
#endif /* __cplusplus */




#include <stdlib.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/klog.h>
#include "reg_ctrl.h"

#define READ_REGI(a)		(*(volatile unsigned int *)(a))
#define WRITE_REGI(a, v)	(*(volatile unsigned int *)(a) = (v))

struct reg_field *regmap_field_alloc(unsigned int base, struct reg_field reg_field)
{
	struct reg_field * filed = calloc(1, sizeof(struct reg_field));
	if (filed == NULL) {
		klog("%s() Error! no memory!\n", __func__);
		return NULL;
	}

	memcpy(filed, &reg_field, sizeof(struct reg_field));
	filed->reg += (unsigned int)base;
	filed->shift = reg_field.lsb;
	filed->mask = GENMASK(reg_field.msb, reg_field.lsb);
	return filed;
}

void regmap_field_free(struct reg_field *field)
{
	if (field) {
		free(field);
	}
}

int regmap_field_read(struct reg_field *field, unsigned int *val)
{
	unsigned int reg_val = READ_REGI(field->reg);
	reg_val &= field->mask;
	reg_val >>=field->shift;
	*val = reg_val;
	return 0;
}

int regmap_field_write(struct reg_field *field, unsigned int val)
{
	unsigned reg_val = READ_REGI(field->reg);
	unsigned int set_val = ((reg_val & ~(field->mask)) | ((val << field->shift) & field->mask));
	WRITE_REGI(field->reg, set_val);
	return 0;
}

int regmap_read(unsigned int base, unsigned int reg_off, unsigned int *val)
{
	*val = READ_REGI((base + reg_off));
	return 0;
}

int regmap_write(unsigned int base, unsigned int reg_off, unsigned int val)
{
	WRITE_REGI(base + reg_off, val);
	return 0;
}
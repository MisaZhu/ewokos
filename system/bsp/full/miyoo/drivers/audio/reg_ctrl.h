
#ifndef REG_FILED_CTRL_H_
#define REG_FILED_CTRL_H_

struct reg_field {
	unsigned int mask;
	unsigned int shift;
	/* Basic descriptor */
	unsigned int reg;
	unsigned int lsb;
	unsigned int msb;
	unsigned int id_size;
	unsigned int id_offset;
};

#define REG_FIELD(_reg, _lsb, _msb) {   \
   	.reg = _reg,  \
   	.lsb = _lsb,  \
	.msb = _msb,  \
}

#define BITS_PER_LONG 32
#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

struct reg_field *regmap_field_alloc(unsigned int base, struct reg_field reg_field);
void regmap_field_free(struct reg_field *field);

int regmap_field_read(struct reg_field *field, unsigned int *val);
int regmap_field_write(struct reg_field *filed, unsigned int val);

int regmap_read(unsigned int base, unsigned int reg_off, unsigned int *val);
int regmap_write(unsigned int base, unsigned int reg_off, unsigned int val);

#endif
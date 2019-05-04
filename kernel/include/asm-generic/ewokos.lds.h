/* Align . to a 8 byte boundary equals to maximum function alignment. */
#define ALIGN_FUNCTION() . = ALIGN(8)

#define HEAD_TEXT						\
	*(.head.text)
#define HEAD_TEXT_SECTION					\
	.head.text : AT(ADDR(.head.text)) {			\
		HEAD_TEXT					\
	}
#define INIT_DATA						\
	*(.init.data)						\
	*(.init.rodata)
#define INIT_TEXT						\
	*(.init.text)
#define INITCALLS						\
  	*(.initcall0.init)					\
  	*(.initcall0s.init)					\
  	*(.initcall1.init)					\
  	*(.initcall1s.init)					\
  	*(.initcall2.init)					\
  	*(.initcall2s.init)					\
  	*(.initcall3.init)					\
  	*(.initcall3s.init)					\
  	*(.initcall4.init)					\
  	*(.initcall4s.init)					\
  	*(.initcall5.init)					\
  	*(.initcall5s.init)
#define INIT_CALLS						\
	__initcall_start = .;					\
	INITCALLS						\
	__initcall_end = .;
#define TEXT_TEXT						\
	ALIGN_FUNCTION();					\
	*(.text)						\
	*(.ref.text)
#define DATA_DATA						\
	*(.data)						\
	*(.ref.data)

/*
 * Read only Data
 */
#define RO_DATA_SECTION(align)					\
	. = ALIGN((align));					\
	.rodata : AT(ADDR(.rodata)) {				\
		__start_rodata = .;				\
		*(.rodata) *(.rodata.*)				\
	}							\
	/* __*init sections */					\
	__init_rodata : AT(ADDR(__init_rodata)) {		\
		*(.ref.rodata)					\
	}							\
	__end_rodata = .;					\
	. = ALIGN((align));
/*
 * bss (Block Started by Symbol) - uninitialized data
 * zeroed during startup
 */
#define BSS(bss_align)						\
	. = ALIGN(bss_align);					\
	.bss : AT(ADDR(.bss)) {					\
		*(.bss)						\
	}
#define BSS_SECTION(bss_align, stop_align)			\
	. = ALIGN(bss_align);					\
	__bss_start = .;					\
	BSS(bss_align)						\
	. = ALIGN(stop_align);					\
	__bss_stop = .;

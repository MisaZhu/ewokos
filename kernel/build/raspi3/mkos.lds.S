ENTRY(__entry)

base_address = 0x80000000;
start_address = 0x8000; /*for real hardware*/

PHDRS
{
  init   PT_LOAD FLAGS(5); /* read-only, executable*/
  text   PT_LOAD FLAGS(5); /* read-only, executable*/
}

SECTIONS
{
	/*bootloader will be compiled with phy address base at 0x8000*/
	.init start_address : AT(start_address) ALIGN(4096)
  {
    KEEP(*(.init .init.*))
		../../basic/arch/sys/arm/v7/start.o(.text .text.*)
		../../basic/arch/sys/arm/v7/start.o(.rodata .rodata.*)
		../../basic/arch/sys/arm/v7/start.o(.data .data.*)
		../../basic/arch/sys/arm/v7/start.o(.bss .bss.*)
  } : init
  _init_end = ALIGN(4096);

	/*kernel will be compiled with virtual address base at 2GB*/
	PROVIDE(_kernel_start = base_address + _init_end);

	.text base_address + _init_end : AT(_init_end) ALIGN(4096)
	{
		KEEP(*(.text))
		*(.text)
	} : text
	. = ALIGN(4096);

	.data : { /** data segment */
		*(.data*)
	}
	. = ALIGN(4096);

	_bss_start = .;
	.bss : 
	{ 
		*(.bss*)
	}
	. = ALIGN(4096);
	_bss_end = .;

	. = . + 0x4000;
	_irq_stack = .;

	. = . + 0x4000;
	_abt_stack = .;

	. = . + 0x4000;
	_undef_stack = .;

	. = . + 0x4000;
	_svc_stack = .;
	PROVIDE(_kernel_sp = .);

	. = ALIGN(4096);
	PROVIDE(_kernel_end = .);
}

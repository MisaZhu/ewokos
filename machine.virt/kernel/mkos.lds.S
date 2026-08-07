ENTRY(__entry)

base_address = 0x4000000000;

PHDRS
{
  init   PT_LOAD FLAGS(5); /* read-only, executable*/
  text   PT_LOAD FLAGS(5); /* read-only, executable*/
}

SECTIONS
{
	.init start_address : AT(start_address) ALIGN(page_align_size)
	{
    KEEP(*(.init .init.*))
		*/start*.o(.text .text.*)
		*/start*.o(.rodata .rodata.*)
		*/start*.o(.data .data.*)
		*/start*.o(.bss .bss.*)
	} : init
	. = . + 0x4000;
	_svc_start_stack = .;
	_init_end = ALIGN(page_align_size);

	/*kernel will be compiled with virtual address base at 256GB*/
	PROVIDE(_kernel_start = base_address + _init_end - start_address);

	.text base_address + _init_end - 0x40000000: AT(_init_end) ALIGN(page_align_size)
	{
		KEEP(*(.text))
		*(.text)
	} : text
	. = ALIGN(page_align_size);

	.data : { /** data segment */
		*(.data*)
	}
	. = ALIGN(page_align_size);

	_bss_start = .;
	.bss : 
	{ 
		*(.bss*)
	}
	. = ALIGN(page_align_size);
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

	. = ALIGN(page_align_size);
	PROVIDE(_kernel_end = .);
}

ENTRY(__entry)

INCLUDE linker.lds.S

PHDRS
{
  init   PT_LOAD FLAGS(5); /* read-only, executable */
  text   PT_LOAD FLAGS(5); /* read-only, executable */
}

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/

	.init start_address : AT(start_address) ALIGN(4096)
  {
    KEEP(*(.init .init.*))
  } : init

  _init_end = ALIGN(4096);

	PROVIDE(_kernel_start = base_address + _init_end);

	.text base_address + _init_end : AT(_init_end) ALIGN(4096)
	{
		KEEP(*(.text.boot))
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

	. = . + 0x10000;
	_irq_stack = .;

	. = . + 0x10000;
	_abt_stack = .;

	. = . + 0x10000;
	_svc_stack = .;

	. = ALIGN(4096);
	PROVIDE(_framebuffer_base_raw = .); 
	. += framebuffer_size;
	PROVIDE(_framebuffer_end_raw = .); 

	. = ALIGN(4096);
	PROVIDE(_kernel_end = .);
}

ENTRY(__entry)

INCLUDE linker.lds.S

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/
	. = base_address + start_address;

	PROVIDE(_kernel_start = .);

	.text : AT(start_address)
	{
		KEEP(*(.text.boot))
		*(.text)
	}
	. = ALIGN(4096);

	.data : { /** data segment */
		*(.data*)
	}
	. = ALIGN(4096);

	.bss : 
	{ 
		*(.bss*)
	}
	. = ALIGN(4096);

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

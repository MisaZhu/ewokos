ENTRY(__entry) /*kernel entry (kernel/asm/boot.S)*/

/*kernel will be loaded at this address after boot*/
INCLUDE linker.ld

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/
	. = 0x80000000 + start_address; /*2GB + start_address = (0x80010000)*/
	PROVIDE(_kernelStart = .); /*defined at kernel/include/kernel.h, set to 0x80010000*/

	.text : {
		KEEP(*(.text.boot))
			*(.text)
	}
	. = ALIGN(4096);

	.rodata : {
		*(.rodata)
	}
	. = ALIGN(4096);

	.data : {
		*(.data)
	}
	. = ALIGN(4096);

	.bss : {
		*(.bss COMMON)
	}
	. = ALIGN(4096);

/* .text : AT(start_address) 
	{
		*(.text)
	}

	.data : 
	{ 
		*(.data) 
	}

	.bss : 
	{ 
		*(.bss COMMON)
	}
	. = ALIGN(8);
*/

	PROVIDE(_kernelEnd = .); /*defined at kernel/include/kernel.h*/
}

ENTRY(__entry) /*kernel entry (kernel/asm/boot.S)*/

/*kernel will be loaded at this address after boot*/
INCLUDE linker.ld

SECTIONS
{
	/*kernel will be compiled with virtual address base at 2GB*/
	. = 0x80000000 + start_address; /*2GB + start_address = (0x80010000)*/
	PROVIDE(_kernel_start = .); /*defined at kernel/include/kernel.h, set to 0x80010000*/

 .text : AT(start_address) 
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
	PROVIDE(_fb_start = .); 
	. += framebuffer_size;

	PROVIDE(_kernel_end = .); /*defined at kernel/include/kernel.h*/
}

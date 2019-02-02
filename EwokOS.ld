ENTRY(__entry) /*kernel entry (kernel/asm/boot.S)*/

start_address = 0x10000; /*kernel will be loaded at this address after boot*/

SECTIONS
{
	. = 0x80000000 + start_address; /*2GB + start_address*/
	PROVIDE(_kernelStart = .); /*defined at kernel/include/kernel.h, set to 2GB + start_address */

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

	PROVIDE(_kernelEnd = .); /*defined at kernel/include/kernel.h*/
}

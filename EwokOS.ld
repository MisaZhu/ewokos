ENTRY(__entry)

start_address = 0x10000;

SECTIONS
{
	. = 0x80000000 + start_address;

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

	PROVIDE(_kernelEnd = .);
}

ENTRY(__entry)

base_address = DEFINED(kernel_base) ? kernel_base : 0x4000000000;

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

	/* AP stacks: boot.S slave_core sets SP = _svc_stack + cid*0x4000
	 * (one 16K slot per secondary, 3 secondaries on virt).  Reserve the
	 * slots INSIDE the image: past _kernel_end the kernel places the
	 * vsyscall page (mapped Device in kernel_vm, so AP stack writes made
	 * through the cacheable boot alias read back as 0 after the VM
	 * switch) and the kernel page-table pool. */
	. = . + 0x4000 * 3;

	. = ALIGN(page_align_size);
	PROVIDE(_kernel_end = .);
}

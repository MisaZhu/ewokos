/*
 * Automatically generated C config: don't edit
 * EwokOS version: 1.0.0.0
 * Thu Jun 20 02:00:46 2019
 */
/* Three system performance ? should be considered carefully to choose the
 * software architecture model:
 * 1. throughput: IO throughput should be as great as possible
 * 2. realtime: time critical tasks can be run on time
 * 3. faireness: tasks can be distributed fairly on difference computing
 *               resources
 * Systems will always take throughput into account, realtime and fairness
 * are left for users' choices.
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__entry)
/*kernel will be loaded at this address after boot*/
INCLUDE linker.lds.S
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

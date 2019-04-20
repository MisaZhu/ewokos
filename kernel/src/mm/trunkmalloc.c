#include <mm/trunkmalloc.h>
#include <mm/mmu.h>
#include <proc.h>

/*
malloc for memory trunk management
*/

static mem_block_t* gen_block(char* p, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	mem_block_t* block = (mem_block_t*)p;
	block->next = block->prev = 0;
	block->mem = p + block_size;
	block->size = size - block_size;
	return block;
}

/*if block size much bigger than the size required, break to two blocks*/
static void try_break(malloc_t* m, mem_block_t* block, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	//required more than half size of block. no break.
	if((block_size+size) > (block->size-(block_size*2))) 
		return;
	
	//do break;
	size = ALIGN_UP(size, 4);
	char* p = block->mem + size;
	mem_block_t* newBlock = gen_block(p, block->size - size);
	newBlock->used = 0; //break a new free block.

	block->size = size;
	newBlock->next = block->next;
	if(newBlock->next != 0)
		newBlock->next->prev = newBlock;

	newBlock->prev = block;
	block->next = newBlock;

	if(m->mTail == block) 
		m->mTail = newBlock;
}

char* trunk_malloc(malloc_t* m, uint32_t size) {
	mem_block_t* block = m->mHead;
	while(block != NULL) {
		if(block->used || block->size < size) {
			block = block->next;
		}
		else {
			block->used = 1;
			try_break(m, block, size);
			break;
		}
	}
	if(block != NULL) {
		return block->mem;
	}

	/*Can't find any available block, expand pages*/
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t expand_size = size + block_size;

	uint32_t pages = expand_size / PAGE_SIZE;	
	if((expand_size % PAGE_SIZE) > 0)
		pages++;

	char* p = (char*)m->get_mem_tail(m->arg);
	if(!m->expand(m->arg, pages))
		return NULL;

	block = gen_block(p, pages*PAGE_SIZE);
	block->used = 1;

	if(m->mHead == 0) {
		m->mHead = block;
	}

	if(m->mTail == 0) {
		m->mTail = block;
	}
	else {
		m->mTail->next = block;
		block->prev = m->mTail;
		m->mTail = block;
	}

	try_break(m, block, size);
	return block->mem;
}

/*
try to merge around free blocks.
*/
static void try_merge(malloc_t* m, mem_block_t* block) {
	uint32_t block_size = sizeof(mem_block_t);
	//try next block	
	mem_block_t* b = block->next;
	if(b != NULL && b->used == 0) {
		block->size += (b->size + block_size);
		block->next = b->next;
		if(block->next != NULL) 
			block->next->prev = block;
		else
			m->mTail = block;
	}

	//try left block	
	b = block->prev;
	if(b != NULL && b->used == 0) {
		b->size += (block->size + block_size);
		b->next = block->next;
		if(b->next != NULL) 
			b->next->prev = b;
		else
			m->mTail = b;
	}
}

/*
try to shrink the pages.
*/
static void try_shrink(malloc_t* m) {
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t addr = (uint32_t)m->mTail;
	//check if page aligned.	
	if(m->mTail == NULL || m->mTail->used == 1 || (addr % PAGE_SIZE) != 0)
		return;

	int pages = (m->mTail->size+block_size) / PAGE_SIZE;
	m->mTail = m->mTail->prev;
	if(m->mTail != NULL)
		m->mTail->next = NULL;
	//else
	//	m->mTail = m->mHead;
	else
		m->mHead = NULL;
	m->shrink(m->arg, pages);
}

void trunk_free(malloc_t* m, char* p) {
	uint32_t block_size = sizeof(mem_block_t);
	if(((uint32_t)p) < block_size) //wrong address.
		return;
	mem_block_t* block = (mem_block_t*)(p-block_size);
	block->used = 0; //mark as free.
	try_merge(m, block);
	if(m->shrink != NULL)
		try_shrink(m);
}

#include <mm/pmalloc.h>
#include <mm/mmu.h>

/*
malloc for use memory management
*/

static MemBlockT* genBlock(char* p, uint32_t size) {
	uint32_t blockSize = sizeof(MemBlockT);
	MemBlockT* block = (MemBlockT*)p;
	block->next = block->prev = 0;
	block->mem = p + blockSize;
	block->size = size - blockSize;
	return block;
}

/*if block size much bigger than the size required, break to two blocks*/
static void tryBreak(MallocT* m, MemBlockT* block, uint32_t size) {
	uint32_t blockSize = sizeof(MemBlockT);
	//required more than half size of block. no break.
	if((blockSize + size) > (uint32_t)(block->size/2)) 
		return;
	
	//do break;
	size = ALIGN_UP(size, 4);
	char* p = block->mem + size;
	MemBlockT* newBlock = genBlock(p, block->size - size);
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

char* pmalloc(MallocT* m, uint32_t size) {
	MemBlockT* block = m->mHead;
	while(block != NULL) {
		if(block->used || block->size < size) {
			block = block->next;
		}
		else {
			block->used = 1;
			tryBreak(m, block, size);
			return block->mem;
		}
	}

	/*Can't find any available block, expand pages*/
	uint32_t blockSize = sizeof(MemBlockT);
	uint32_t expandSize = size + blockSize;

	uint32_t pages = expandSize / PAGE_SIZE;	
	if((expandSize % PAGE_SIZE) > 0)
		pages++;

	char* p = (char*)m->getMemTail(m->arg);
	if(!m->expand(m->arg, pages))
		return NULL;

	block = genBlock(p, pages*PAGE_SIZE);
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

	tryBreak(m, block, size);
	return block->mem;
}

/*
try to merge around free blocks.
*/
static void tryMerge(MallocT* m, MemBlockT* block) {
	uint32_t blockSize = sizeof(MemBlockT);
	//try next block	
	MemBlockT* b = block->next;
	if(b != 0 && b->used == 0) {
		block->size += (b->size + blockSize);
		block->next = b->next;
		if(block->next != 0) 
			block->next->prev = block;

		if(m->mTail == b)
			m->mTail = block;
	}

	//try left block	
	b = block->prev;
	if(b != 0 && b->used == 0) {
		b->size += (block->size + blockSize);
		b->next = block->next;
		if(b->next != 0) 
			b->next->prev = b;
		if(m->mTail == block)
			m->mTail = b;
	}
}

/*
try to shrink the pages.
*/
static void tryShrink(MallocT* m) {
	uint32_t blockSize = sizeof(MemBlockT);
	uint32_t addr = (uint32_t)m->mTail;
	//check if page aligned.	
	if(m->mTail->used == 1 || (addr % PAGE_SIZE) != 0)
		return;

	int pages = (m->mTail->size+blockSize) / PAGE_SIZE;
	m->mTail = m->mTail->prev;
	m->mTail->next = 0;
	if(m->mTail == 0)
		m->mHead = 0;

	m->shrink(m->arg, pages);
}

void pfree(MallocT* m, char* p) {
	uint32_t blockSize = sizeof(MemBlockT);
	if(((uint32_t)p) < blockSize) //wrong address.
		return;
	MemBlockT* block = (MemBlockT*)(p-blockSize);
	block->used = 0; //mark as free.

	tryMerge(m, block);
	tryShrink(m);
}

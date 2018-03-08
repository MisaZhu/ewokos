#include <kfile.h>
#include <kmalloc.h>
#include <string.h>

static KFileT* _kernelFiles = NULL;

KFileT* kfOpen(uint32_t fsNodeAddr) {
	KFileT* kf = (KFileT*)kmalloc(sizeof(KFileT));	
	if(kf == NULL)
		return NULL;

	memset(kf, 0, sizeof(KFileT));
	if(_kernelFiles == NULL) {
		_kernelFiles = kf;
	}
	else {
		kf->next = _kernelFiles;
		_kernelFiles->prev = kf;
		_kernelFiles = kf;
	}

	kf->fsNodeAddr = fsNodeAddr;
	return kf;
}

void kfUnref(KFileT* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	if((flags & KF_WRITE) == 0) {//read mode
		if(kf->refR > 0)
			kf->refR--;
	}
	else {
		if(kf->refW > 0)
			kf->refW--;
	}

	if((kf->refW + kf->refR) == 0) { // close when ref cleared.
		if(kf->prev != NULL)
			kf->prev->next = kf->next;
		if(kf->next != NULL)
			kf->next->prev = kf->prev;

		if(kf == _kernelFiles)
			_kernelFiles = kf->next;
		kmfree(kf);
	}
}

void kfRef(KFileT* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	if((flags & KF_WRITE) == 0) //read mode
		kf->refR++;
	else
		kf->refW++;
}

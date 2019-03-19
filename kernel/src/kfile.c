#include <kfile.h>
#include <kstring.h>
#include <mm/kmalloc.h>

KFileT* kfOpen(uint32_t nodeAddr) {
	KFileT* kf = (KFileT*)kmalloc(sizeof(KFileT));	
	if(kf == NULL)
		return NULL;

	memset(kf, 0, sizeof(KFileT));
	kf->nodeAddr = nodeAddr;
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

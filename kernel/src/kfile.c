#include <kfile.h>
#include <kstring.h>
#include <mm/kmalloc.h>

k_file_t* kf_open(uint32_t node_addr) {
	k_file_t* kf = (k_file_t*)km_alloc(sizeof(k_file_t));	
	if(kf == NULL)
		return NULL;

	memset(kf, 0, sizeof(k_file_t));
	kf->node_addr = node_addr;
	return kf;
}

void kf_unref(k_file_t* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	if((flags & KF_WRITE) == 0) {//read mode
		if(kf->ref_r > 0)
			kf->ref_r--;
	}
	else {
		if(kf->ref_w > 0)
			kf->ref_w--;
	}

	if((kf->ref_w + kf->ref_r) == 0) { // close when ref cleared.
		km_free(kf);
	}
}

void kf_ref(k_file_t* kf, uint32_t flags) {
	if(kf == NULL)
		return;

	if((flags & KF_WRITE) == 0) //read mode
		kf->ref_r++;
	else
		kf->ref_w++;
}

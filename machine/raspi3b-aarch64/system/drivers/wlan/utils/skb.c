#include <unistd.h>
#include <stdlib.h>
#include <types.h>

#include "skb.h"
#include "log.h"

struct sk_buff* skb_alloc(uint32_t size){
    struct sk_buff *skb = malloc(sizeof(struct sk_buff));
    if(!skb)
        return NULL;

    skb->len = 0;
    skb->total =  size + SKB_MAX_EXTEND * 2;
    skb->mem = calloc(1, skb->total);
    if(!skb->mem){
        free(skb);
        return NULL;
    }
    skb->data = skb->mem + SKB_MAX_EXTEND;
    return skb;
}

void *skb_put(struct sk_buff* skb, uint32_t size){
    if(!skb)
        return NULL;
    void* ret = skb->data;
    skb->len += size;
    if(skb->data + skb->len > skb->mem + skb->total){
        brcm_log("reach skb buffer max extend:%d \n", SKB_MAX_EXTEND);
    }
    return ret;
}

void *skb_push(struct sk_buff* skb, uint32_t size){
    if(!skb)
        return NULL;
    void* ret = skb->data;
    skb->len += size;
    skb->data -= size;
    if(skb->data < skb->mem){
        brcm_log("reach skb buffer min extend:%d \n", SKB_MAX_EXTEND);
    }
    return ret;
}

void *skb_pull(struct sk_buff* skb, uint32_t size){
    if(!skb)
        return NULL;
    void* ret = skb->data;
    skb->len -= size;
    skb->data += size;
    if(skb->data > skb->mem + skb->total){
        brcm_log("reach skb buffer max extend:%d \n", SKB_MAX_EXTEND);
    }
    return ret;
}

void skb_reserve(struct sk_buff* skb, uint32_t size){
    if(!skb)
        return;
    skb->data += size;
    if(skb->data > skb->mem + skb->total){
        brcm_log("reach skb buffer max extend:%d \n", SKB_MAX_EXTEND);
    }
}

void *skb_trim(struct sk_buff* skb, uint32_t size){
    if(!skb && size >= skb->len)
        return NULL;
    skb->len = size;
    return skb->data;
}

void skb_free(struct sk_buff* skb){
    if(!skb)
        return;
    if(skb->mem)
        free(skb->mem);
    free(skb);
}
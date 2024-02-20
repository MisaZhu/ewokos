#include <unistd.h>

#include "skb.h"


void skb_put(struct sk_buff* skb, int size){
    if(!skb)
        return;
    skb->len += size;
    if(!skb->data || skb->data == skb->buf){
        skb->data = malloc(skb->len); 
        memcpy(skb->data, skb->buf, SKB_PRE_ALLOC_SIZE);
    }else{
        skb->data = realloc(skb->data, skb->len);
    }

}

struct sk_buff* dev_alloc_skb(int size){
    struct sk_buff *skb = malloc(sizeof(struct sk_buff));
    if(!skb)
        return NULL;

    if(size > SKB_PRE_ALLOC_SIZE){
        int msize = (size + SKB_PRE_ALLOC_SIZE - 1)&(~(SKB_PRE_ALLOC_SIZE - 1));
        skb->data = malloc(msize);
        if(!skb->data){
            free(skb);
            return NULL;
        }
    }else if(size){
         skb->data = skb->buf;
    }
    else{
        skb->data = NULL;
    }

    skb->len = size;
    return skb;
}

void dev_kfree_skb(struct sk_buff* skb){
    if(!skb)
        return;
    if(skb->data && skb->data != skb->buf)
        free(skb->data);
    free(skb);
}

void skb_trim(struct sk_buff* skb){
    if(!skb)
        return;
    if(skb->data && skb->data != skb->buf){
        free(skb->data);
    }
    skb->data = NULL;
    skb->len = 0;
}
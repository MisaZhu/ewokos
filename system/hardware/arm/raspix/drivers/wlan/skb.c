#include <unistd.h>

#include "skb.h"


void skb_put(struct sk_buff* skb, int size){
    if(!skb)
        return;
    skb->len += size;
    if(skb->data)
        skb->data = realloc(skb->data, skb->len);
    else
        skb->data = malloc(skb->len); 
}

struct sk_buff* dev_alloc_skb(int size){
    struct sk_buff *skb = malloc(sizeof(struct sk_buff));
    if(!skb)
        return NULL;
    if(size){
        size+=(64-1);
        size&=~(64-1);
        skb->data = malloc(size);
        if(!skb->data){
            free(skb);
            return NULL;
        }
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
    if(skb->data)
        free(skb->data);
    free(skb);
}

void skb_trim(struct sk_buff* skb){
    if(!skb)
        return;
    if(skb->data){
        free(skb->data);
        skb->data = NULL;
    }
    skb->len = 0;
}
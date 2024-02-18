#ifndef __SKB_H__
#define __SKB_H__
#include "types.h"

struct sk_buff{
    int priority;
    uint32_t len;
    uint8_t* data;
}; 

void skb_put(struct sk_buff* skb, int size);
struct sk_buff* dev_alloc_skb(int size);
void dev_kfree_skb(struct sk_buff* skb);

#endif
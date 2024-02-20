#ifndef __SKB_H__
#define __SKB_H__
#include "types.h"

#define SKB_PRE_ALLOC_SIZE  16

struct sk_buff{
    int priority;
    uint32_t len;
    uint8_t* data;
    uint8_t buf[SKB_PRE_ALLOC_SIZE];
}; 

struct sk_buff_head {
	struct_group_tagged(sk_buff_list, list,
		struct sk_buff	*next;
		struct sk_buff	*prev;
	);
	uint32_t		qlen;
};


void skb_put(struct sk_buff* skb, int size);
struct sk_buff* dev_alloc_skb(int size);
void dev_kfree_skb(struct sk_buff* skb);

#endif
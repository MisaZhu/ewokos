#ifndef __SKB_H__
#define __SKB_H__

#include <types.h>

#define SKB_MAX_EXTEND 2048 

struct sk_buff{
    int priority;
    uint32_t len;
    uint8_t *data;
	//internal
	uint8_t *mem;
	uint32_t total;
}; 

struct sk_buff_head {
	struct_group_tagged(sk_buff_list, list,
		struct sk_buff	*next;
		struct sk_buff	*prev;
	);
	uint32_t		qlen;
};



struct sk_buff* skb_alloc(uint32_t size);
void *skb_put(struct sk_buff* skb, uint32_t size);
void *skb_push(struct sk_buff* skb, uint32_t size);
void *skb_pull(struct sk_buff* skb, uint32_t size);
void *skb_trim(struct sk_buff* skb, uint32_t size);
void skb_reserve(struct sk_buff* skb, uint32_t size);
void skb_free(struct sk_buff* skb);

#endif
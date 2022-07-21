
#ifndef _TEMP_LIST_H_
#define _TEMP_LIST_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct listnode
{
    struct listnode *next;
    struct listnode *prev;
};

#define node_to_item(node, container, member) \
    (container *) (((char*) (node)) - offsetof(container, member))

#define list_declare(name) \
    struct listnode name = { \
        .next = &(name), \
        .prev = &(name), \
    }

#define list_for_each(node, list) \
    for ((node) = (list)->next; (node) != (list); (node) = (node)->next)

#define list_for_each_reverse(node, list) \
    for ((node) = (list)->prev; (node) != (list); (node) = (node)->prev)

#define list_for_each_safe(node, n, list) \
    for ((node) = (list)->next, (n) = (node)->next; \
         (node) != (list); \
         (node) = (n), (n) = (node)->next)

static inline void list_init(struct listnode *node)
{
    node->next = node;
    node->prev = node;
}

static inline void list_add_tail(struct listnode *head, struct listnode *item)
{
    item->next = head;
    item->prev = head->prev;
    head->prev->next = item;
    head->prev = item;
}

static inline void list_add_head(struct listnode *head, struct listnode *item)
{
    item->next = head->next;
    item->prev = head;
    head->next->prev = item;
    head->next = item;
}

static inline void list_remove(struct listnode *item)
{
    item->next->prev = item->prev;
    item->prev->next = item->next;
}

#define list_empty(list) ((list) == (list)->next)
#define list_head(list) ((list)->next)
#define list_tail(list) ((list)->prev)

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif

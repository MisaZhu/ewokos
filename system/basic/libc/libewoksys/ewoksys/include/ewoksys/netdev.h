#ifndef EWOKSYS_NETDEV_H
#define EWOKSYS_NETDEV_H

/* Optional Ethernet RX batching, version 1. dev_cntl(QUERY) returns V1 as
 * one integer only when supported. A dev_read at OFFSET then returns up
 * to 16 [u16 little-endian length][Ethernet frame] records. Ordinary reads
 * retain their single-frame format. No global mode or reader state changes.
 */
#define NETDEV_RX_BATCH_QUERY 0x4e525800
#define NETDEV_RX_BATCH_V1    0x4e525801
/* Negative so an ordinary sequential reader cannot reach it by advancing. */
#define NETDEV_RX_BATCH_OFFSET (-NETDEV_RX_BATCH_V1)
#define NETDEV_FRAME_MAX     1514u
#define NETDEV_RX_BATCH_FRAMES 16u
#define NETDEV_RX_BATCH_BYTES (NETDEV_RX_BATCH_FRAMES * (2u + NETDEV_FRAME_MAX))

#endif

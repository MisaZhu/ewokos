#ifndef POLL_H
#define POLL_H

struct pollfd {
    int      fd;       
    uint16_t events;   
    uint16_t revents;  
};

#endif

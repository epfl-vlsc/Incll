#pragma once

#define PPP_HEADER_SIZE 16

class PPP{
public:
    void *next;
    void *tmp;

    void *operator=(void *n){
        next=n;
        tmp=n;
        return next;
    }

    void *operator=(PPP &n){
        return (*this)=(void*)n;
    }

    operator void*(){
        assert(next == tmp);
        return next;
    }
};



/* Modification of Masstree
 * VLSC Laboratory
 * Copyright (c) 2018-2019 Ecole Polytechnique Federale de Lausanne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */

#pragma once
#include "compiler.hh"

#define PPP_HEADER_SIZE 16

bool epoch_is_valid(unsigned long e);

typedef uint64_t mrcu_epoch_type;
extern volatile mrcu_epoch_type globalepoch;
extern volatile mrcu_epoch_type currexec;

class PPP{
    unsigned long next;
    unsigned long nextLog;
public:
    void *read(){
        unsigned long n = next;
        unsigned long nl = nextLog;
        if( likely((n&1) == (nl&1)) ){
            unsigned long epoch = n>>48 | ((nl>>32)&0xFF00);
            if(epoch < currexec && epoch_is_valid(epoch)==false){
                return (void*)(nl & 0x0000FFFFFFFFFFF0ul); //TODO: also fix the node.
            }
            return (void*)(n & 0x0000FFFFFFFFFFF0ul);
        }
        else{
            return (void*)(nl & 0x0000FFFFFFFFFFF0ul); //TODO: also fix the node.
        }
    }
    void write(void *val){
        unsigned long n = next;
        unsigned long nl = nextLog;
        unsigned long gepoch = globalepoch;
        unsigned long ehigh = (gepoch<<32)&0xFFFF000000000000, elow = (gepoch<<48);
        //previously crashed. Keep (only) the log.
        if( unlikely((n&1) != (nl&1)) ){
            unsigned long valid = (nl&0x1);
            nextLog = ehigh | (nl&0x0000FFFFFFFFFFFFul);//just keep validity. No need to flip
            next = elow | ((unsigned long)val&0x0000FFFFFFFFFFF0ul) | valid;
            return;
        }
        unsigned long epoch = n>>48 | ((nl>>32)&0xFF00);
        if(epoch < currexec && epoch_is_valid(epoch)==false){
            unsigned long valid = (nl&0x1)^0x1;
            nextLog = ehigh | (nl&0x0000FFFFFFFFFFF0ul) | valid ;//^1 flips validity bit
            next = elow | ((unsigned long)val&0x0000FFFFFFFFFFF0ul) | valid;
            return;
        }
        if(epoch == globalepoch){
            next = (n & 0xFFFF00000000000Ful) | ((unsigned long)val&0x0000FFFFFFFFFFF0ul);
            return;
        }
        unsigned long valid = (nl&0x1)^0x1;
        nextLog = ehigh | (n&0x0000FFFFFFFFFFF0ul) | valid;
        next = elow | ((unsigned long)val&0x0000FFFFFFFFFFF0ul) | valid;
    }
    //void *tmp;
    void *operator=(void *n){
        //assert( ((unsigned long)n & 0xF) == 0); //alignment.
        write(n);
        //tmp = n;
        //assert(read()==n);
        return n;
    }
    operator void *(){
        //assert(tmp == read());
        return read();
    }
};



/**********************
 *      Cache.c       *
 *   Powered by Ag2S  *
 *    201900130078    *
 * finish :2020/12/22 *
 *  start: 2020/12/08 *
 * Only for study !!! *
**********************/



#include "cache.h"
#include "shell.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>


/*************************
*    cache initialize    *
**************************/

void init_cache_line(cache_line* line) {
    line->mark = 0;
    line->time = 0;
    line->mem = (uint8_t*)malloc(sizeof(uint8_t)*32);
    memset(line->mem, 0, 32*sizeof(uint8_t));
}

void init_inst_cache(inst_cache_line* line) {
    line->time = 0;
    line->mark = 0;
    line->mem = (uint8_t*)malloc(sizeof(uint8_t)*32);
    memset(line->mem, 0, 32*sizeof(uint8_t));
}

bool valid(cache_line* line) {
    return line->mark & 1;
}

bool need_to_write(cache_line* line) {
    return (line->mark & 2) >> 1;
}

bool inst_valid(inst_cache_line* line) {
    return line->mark & 1;
}

void group_init(cache_group* group) {
    group->lines = (cache_line*)malloc(sizeof(cache_line)*8);
    int i;
    for(i=0; i<8; ++i) {
        init_cache_line(&(group->lines[i]));
    }
}

void inst_group_init(inst_cache_group* group) {
    group->lines = (inst_cache_line*)malloc(sizeof(inst_cache_line)*4);
    int i;
    for(i=0; i<4; ++i) {
        init_inst_cache(&(group->lines[i]));
    }
}


/* defination of cache */


cache_group CACHE[256] = {
    {NULL}
};
inst_cache_group INST_CACHE[64] = {
    {NULL}
};

int is_waiting;
int is_waiting_data;

/* end of defination */


/*cache self check*/

void cache_self_check() {
    int i;
    for(i=0; i<256; ++i) {
        printf("group %d: \n",i);
        int j;
        for(j=0; j<8; ++j) {
            int k;
            printf("group %d line %d: \n",i,j);
            printf("Mark: %08x \n",CACHE[i].lines[j].mark);
            for(k=0; k<32; ++k) {
                printf("%02x ",CACHE[i].lines[j].mem[k]);
            }
            printf("\n");
        }
    }
}

void inst_cache_self_check() {
    int i;
    for(i=0; i<64; ++i) {
        printf("inst group %d: \n",i);
        int j;
        for(j=0; j<4; ++j) {
            int k;
            printf("inst group %d line %d: \n",i,j);
            printf("Mark: %04x \n",INST_CACHE[i].lines[j].mark);
            for(k=0; k<32; ++k) {
                printf("%02x ",INST_CACHE[i].lines[j].mem[k]);
            }
            printf("\n");
        }
    }
}

/* initialize the cache*/
void init_cache() {
    
#ifdef cacheDebug
    printf("INIT CACHE START! \n");
#endif

    int i;
    for(i=0; i<256; ++i) {
        group_init(&CACHE[i]);
    }

    for(i=0; i<64; ++i) {
        inst_group_init(&INST_CACHE[i]);
    }
    is_waiting = 0;
    is_waiting_data = 0;
#ifdef cacheDebug

    cache_self_check();
    inst_cache_self_check();
    
#endif

}

/**************************************
 *             visit cache            *
 *            read_inst_32            *
 *            read&write_32           *
***************************************/

/* some function that widely used*/

//read 4 byte
inline uint32_t read_4_u8(uint8_t* mem, int inner_offset) {
    return (mem[inner_offset+3] << 24) |
           (mem[inner_offset+2] << 16) |
           (mem[inner_offset+1] << 8)  |
           (mem[inner_offset]);
}

//write
inline void write_4_u8(uint8_t* mem, int inner_offset, uint32_t data) {
    mem[inner_offset+3] = (data >> 24) & 0xFF;
    mem[inner_offset+2] = (data >> 16) & 0xFF;
    mem[inner_offset+1] = (data >> 8 ) & 0xFF;
    mem[inner_offset]   = (data >> 0 ) & 0xFF;
}

//copy a line from mem
void copy_line(uint8_t* line, uint32_t address) {
    //start of the line
    address = address & 0xffffffe0;

    int i;
    for(i=0; i<8; ++i) {
        uint32_t data = mem_read_32(address+i*4);
        write_4_u8(line,i*4,data);
    }
}

//write a line from cache back to mem
void write_back(uint8_t* line, uint32_t address) {
    //start of line;
    address = address & 0xffffffe0;

    int i;
    for(i=0; i<8; ++i) {
        uint32_t data =  read_4_u8(line, i*4);
        mem_write_32(address+i*4,data);
    }
}

/*read instruction*/

//read instruction
uint32_t cache_read_inst(uint32_t address) {

    int group = (address<<21)>>26;
    int inner_offset = address & 0x0000001f;
    uint16_t line_mark = (((address<<12)>>23) & 0xFFFF);

#ifdef cacheDebug

    printf("address: %08x \n",address);
    printf("group: %d \n",group);
    printf("inner offeset: %d \n",inner_offset);
    printf("line mark: %04x \n",line_mark);

#endif

    int i;
    uint32_t ans;

    //hit
    if(direct_read_inst(group, inner_offset, line_mark, &ans)) {

#ifdef cacheDebug

        printf("cache hit! get value: %08x \n", ans);
        system("pause");

#endif

        return ans;
    }
    
    //no hit
    int target_line = 0;
    for(i=0; i<4; ++i) {
        if(!inst_valid(&INST_CACHE[group].lines[i])) {
            target_line = i;
            break;
        }
        if(INST_CACHE[group].lines[i].time > INST_CACHE[group].lines[target_line].time){
            target_line = i;
        }
    }

    copy_line(INST_CACHE[group].lines[target_line].mem, address);
    INST_CACHE[group].lines[target_line].time = 0;
    INST_CACHE[group].lines[target_line].mark = ((line_mark & 0XFFFF) << 1) | (0X0001);
    
#ifdef cacheDebug

        printf("cache no hit get value: %08x \n", read_4_u8(INST_CACHE[group].lines[target_line].mem,inner_offset));
        system("pause");

#endif
    is_waiting = 49;
    return read_4_u8(INST_CACHE[group].lines[target_line].mem,inner_offset);
}

//read direct from inst cache if no hit return false
bool direct_read_inst(int group, int inner_offset,uint16_t line_mark, uint32_t* res) {
    int i;
    bool ret = false;
    for(i=0; i<4; ++i) {
        int mk = INST_CACHE[group].lines[i].mark;
        if(mk>>1 == line_mark && inst_valid(&INST_CACHE[group].lines[i])) {
            
            *res = read_4_u8(INST_CACHE[group].lines[i].mem,inner_offset);
                   
            INST_CACHE[group].lines[i].time = 0;
            ret =  true;
        } else if(inst_valid(&INST_CACHE[group].lines[i])) {
            ++INST_CACHE[group].lines[i].time;
        }
    }
    if(ret == false)
        *res = 0;
    return ret;
}


/* read data */

uint32_t cache_read_32(uint32_t address) {
    uint32_t group = (address<<19)>>24;
    uint32_t line_mark = address>>13;
    int inner_offset = address & 0x0000001f;

#ifdef cacheDebug

    printf("address: %08x \n",address);
    printf("group: %d \n",group);
    printf("inner offeset: %d \n",inner_offset);
    printf("line mark: %08x \n",line_mark);
    
#endif

    uint32_t res;
    if(direct_read_cache(group,inner_offset,line_mark,&res)) {
        //hit
        return res;
    }

    //no hit
    int target_line = 0;
    int i;
    for(i=0; i<8; ++i) {
        if(!valid(&CACHE[group].lines[i])) {
            target_line = i;
            break;
        }
        if(CACHE[group].lines[i].time > CACHE[group].lines[target_line].time) {
            target_line = i;
        }
    }

    if(valid(&CACHE[group].lines[target_line]) && need_to_write(&CACHE[group].lines[target_line])) {
       uint32_t mk =  CACHE[group].lines[target_line].mark >> 2;
       write_back(CACHE[group].lines[target_line].mem,mk << 13 | group << 5);
    }

    copy_line(CACHE[group].lines[target_line].mem, address);
    CACHE[group].lines[target_line].time = 0;
    CACHE[group].lines[target_line].mark = (line_mark << 2) | 1;
    //stat_cycles+=49;
    is_waiting_data = 49;
    return read_4_u8(CACHE[group].lines[target_line].mem,inner_offset);
}

bool direct_read_cache(uint32_t group, int inner_offset, uint32_t line_mark,uint32_t* res) {
    int i;
    bool ret = false;;

    for(i = 0; i<8; ++i) {
        uint32_t mk = (CACHE[group].lines[i].mark) >> 2;
        if(mk == line_mark && valid(&CACHE[group].lines[i])) {
            *res = read_4_u8(CACHE[group].lines[i].mem,inner_offset);

            CACHE[group].lines[i].time = 0;
            ret = true;
        } else {
            ++CACHE[group].lines[i].time;
        }
    }

    if(ret == false)
        *res = 0;
    return ret;
}

/* write data */

void cache_write_32(uint32_t address, uint32_t data) {
    uint32_t group = (address<<19)>>24;
    uint32_t line_mark = address>>13;
    int inner_offset = address & 0x0000001f;
    if(direct_write_chace(group,inner_offset,line_mark,data)) {
        return;
    }

     //no hit
    int target_line = 0;
    int i;
    for(i=0; i<8; ++i) {
        if(!valid(&CACHE[group].lines[i])) {
            target_line = i;
            break;
        }
        if(CACHE[group].lines[i].time > CACHE[group].lines[target_line].time) {
            target_line = i;
        }
    }

    if(valid(&CACHE[group].lines[target_line]) && need_to_write(&CACHE[group].lines[target_line])) {
       uint32_t mk =  CACHE[group].lines[target_line].mark >> 2;
       write_back(CACHE[group].lines[target_line].mem,mk << 13 | group << 5);
    }

    copy_line(CACHE[group].lines[target_line].mem, address);
    CACHE[group].lines[target_line].time = 0;
    CACHE[group].lines[target_line].mark = (line_mark << 2) | 3;
    
    is_waiting_data = 49;
    write_4_u8(CACHE[group].lines[target_line].mem,inner_offset,data);
}

bool direct_write_chace(uint32_t group, int inner_offset, uint32_t line_mark,uint32_t data) {
    int i;
    bool ret = false;;

    for(i = 0; i<8; ++i) {
        uint32_t mk = (CACHE[group].lines[i].mark) >> 2;
        if(mk == line_mark && valid(&CACHE[group].lines[i])) {
            write_4_u8(CACHE[group].lines[i].mem,inner_offset,data);
            CACHE[group].lines[i].mark | 2;
            CACHE[group].lines[i].time = 0;
            ret = true;
        } else {
            ++CACHE[group].lines[i].time;
        }
    }

    return ret;
}

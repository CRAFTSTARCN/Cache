/**********************
 *      Cache.h       *
 *   Powered by Ag2S  *
 *    201900130078    *
 * finish :2020/12/22 *
 *  start: 2020/12/08 *
 * Only for study !!! *
**********************/

#ifndef __AG2S__CACHE
#define __AGES__CACHE

#include <stdint.h>
#include <stdbool.h>

//#define cacheDebug

typedef struct cache_line {
    /*one unit 32 to save space, [0,1] to save valid and dirty, others to save line info*/
    uint32_t mark;
    int time;

    uint8_t *mem;

}cache_line;

typedef struct inst_cache_line {
    /*0 bit to save valid, others to save line info */
    uint16_t mark;
    int time;

    uint8_t *mem;

}inst_cache_line;

typedef struct cache_group
{
    cache_line* lines;
}cache_group;

typedef struct inst_cache_group
{
    inst_cache_line* lines;
}inst_cache_group;

/*Cache initialize*/
void init_cache_line(cache_line* line);
void init_inst_cache(inst_cache_line* line);
void group_init(cache_group* group);
void inst_group_init(inst_cache_group* group);

/*valid*/
bool valid(cache_line* line);
bool inst_valid(inst_cache_line* line);

/*data cache neet to write back?*/
bool need_to_write(cache_line* line);

/*initialize all cache*/

void init_cache();

/*global varibles*/

//data cache
extern cache_group CACHE[];
//instruction cache
extern inst_cache_group INST_CACHE[];

/*cache read and write */

//read inst from cache
uint32_t cache_read_inst(uint32_t address);
//read data from cache
uint32_t cache_read_32(uint32_t address);
void cache_write_32(uint32_t address, uint32_t data);

//direct read, if hit give the inst, return true else return false
bool direct_read_inst(int group, int inner_offset, uint16_t line_mark,uint32_t* res);
//direct read, if hit give the data, return true else return false
bool direct_read_cache(uint32_t group, int inner_offset, uint32_t line_mark,uint32_t* res);
//direct write cache if hit return ture, else false
bool direct_write_chace(uint32_t group, int inner_offset, uint32_t line_mark,uint32_t data);


//copy a line from memory
void copy_line(uint8_t* line, uint32_t address);
void write_back(uint8_t* line, uint32_t address);

extern int is_waiting;
extern int is_waiting_data;
#endif
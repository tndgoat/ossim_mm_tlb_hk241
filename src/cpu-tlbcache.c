/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#ifdef CPU_TLB



#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
typedef struct {
   int valid;
   int pid;          // Process ID
   int page_number;  // Page number
   int frame_number; // Frame number
   int last_used;    // Timestamp to implement LRU
} TLBEntry;
int global_timer = 0;

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE *value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   TLBEntry *entries = (TLBEntry*) mp->storage;

   for (int i = 0; i < mp->maxsz / sizeof(TLBEntry); i++) {
      if (entries[i].valid && entries[i].pid == pid && entries[i].page_number == pgnum) {
         *value = entries[i].frame_number;
         entries[i].last_used = ++global_timer;  // Update last used time
         return 0;  // TLB hit
      }
    }
    return -1;  // TLB miss
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   TLBEntry *entries = (TLBEntry*) mp->storage;
   int min_used_index = -1;
   int min_used_time = INT_MAX;
   BYTE data;
   if ( tlb_cache_read(mp, pid, pgnum, data) == 0) return 0; // HIT
   for (int i = 0; i < mp->maxsz / sizeof(TLBEntry); i++) {
        if (!entries[i].valid) {  // Find an empty slot
            entries[i] = (TLBEntry){1, pid, pgnum, value, ++global_timer};
            return 0;  // New entry added
        }
        if (entries[i].last_used < min_used_time) {  // Track least recently used
            min_used_time = entries[i].last_used;
            min_used_index = i;
        }
    }
    
   // No empty slot found, replace least recently used
   entries[min_used_index] = (TLBEntry){1, pid, pgnum, value, ++global_timer};
   return -1; // MISS
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   TLBEntry *entries = (TLBEntry *)mp->storage;
   printf("======== TLB MEMORY PHYSIC DUMP ========\n");
   for (int i = 0; i < mp->maxsz / sizeof(TLBEntry); i++) {
      if ( entries[i].valid){
         printf("Pid %d pgnum %d: %d\n", entries[i].pid, entries[i].page_number, entries[i].frame_number);
      }
   }
   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (TLBEntry *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;
   TLBEntry *entries = (TLBEntry *)mp->storage;
   for (int i = 0; i < mp->maxsz / sizeof(TLBEntry); i++) {
      entries[i].valid = 0;
      entries[i].pid = -1;
      entries[i].page_number = -1;
      entries[i].frame_number = -1;
      entries[i].last_used = 0;
   }

   return 0;
}

#endif

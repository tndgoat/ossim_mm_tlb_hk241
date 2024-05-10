/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */
  tlb_flush_tlb_of(proc, mp);
  //We don't really use this function
  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/
  if (proc == NULL || mp == NULL || mp->used_fp_list == NULL) {
      // Return an error code to indicate invalid input parameters
      return -1;
  }
  struct framephy_struct *tlb_cache = mp->used_fp_list;

  // Iterate through the TLB cache
  while (tlb_cache != NULL) {
      // Check if the TLB entry is associated with the provided process (proc)
      if (tlb_cache->owner == proc->mm) {
          // Invalidate the TLB entry by setting its owner to NULL
          MEMPHY_remove_usedfp(mp, tlb_cache->fpn);
          MEMPHY_put_freefp(mp, tlb_cache->fpn);
      }
      tlb_cache = tlb_cache->fp_next;
  }

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;
  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  if (val == 0) { // Allocation successful
      // Update TLB CACHED frame number of the new allocated page(s)
      struct memphy_struct *tlb = proc->tlb;
      int pid = proc->pid;
      int pgnum = reg_index; 

      // Assuming newly allocated page(s) have frame number 1
      BYTE new_frame_number = 1;

      // Update TLB cache with the new frame number
      tlb_cache_write(tlb, pid, pgnum, new_frame_number);

      // Print status
      printf("Memory allocated successfully for Process %d - size: %u, address: %d\n", proc->pid, size, addr);
  } else {
      // Print error if memory allocation fails
      printf("Memory allocation failed for Process %d - size: %u\n", proc->pid, size);
  }

  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
    
  // Update TLB CACHED frame number of freed page(s)
  struct memphy_struct *tlb = proc->tlb;
  int pid = proc->pid; 
  int pgnum = reg_index;
  
  // Read TLB cache to obtain the cached frame number
  BYTE cached_frame_number;
  if (tlb_cache_read(tlb, pid, pgnum, &cached_frame_number) == 0) { // Check if cached frame number is valid
      // Update TLB cache with a new frame number
      tlb_cache_write(tlb, pid, pgnum, -1);
  } else {
      // Handling error if TLB cache read fails
      return -1;
  }

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data, frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/
  
	frmnum = tlb_cache_read(proc->tlb, proc->pid, source, &data);
  int val = __read(proc, 0, source, offset, &data);
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if(frmnum < 0){
    if(tlb_cache_write(proc->tlb, proc->pid, source, destination) < 0){
      tlb_flush_tlb_offset(proc, proc->tlb);
    }
  }

  destination = (uint32_t) data;

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  BYTE frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/
  frmnum = tlb_cache_write(proc->tlb, proc->pid, destination, data);
  val = __write(proc, 0, destination, offset, data);
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
   if(frmnum < 0){
    if(tlb_cache_write(proc->tlb, proc->pid, destination, data) < 0){
      tlb_flush_tlb_offset(proc, proc->tlb);
    }
  }


  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  return val;
}

//#endif

/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#ifdef MM_PAGING
/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0)
      return -1;

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }

    return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL)
     return -1;

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp);

   return 0;
}

int MEMPHY_dump(struct memphy_struct * mp)
{
	/*TODO dump memphy contnt mp->storage
 	* 	for tracing the memory content
 	*/
    	printf("=======MEMORY PHYSIC DUMP=======\n");
    	for ( int addr = 0; addr < mp->maxsz; addr++){
            	printf("0x%081x: %08x\n", addr, mp->storage[addr]);
    	}
    	printf("=================================\n");
    	return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;

   return 0;
}

int MEMPHY_remove_usedfp(struct memphy_struct *mp, int fpn){
   struct framephy_struct *fp = mp->used_fp_list;
   if (!fp) return -1;
   if (fp->fpn == fpn) {
      mp->used_fp_list = mp->used_fp_list->fp_next;
      fp->fp_next = NULL;
      return 0;
   }
   else {
      while (fp->fp_next) {
         if (fp->fp_next->fpn == fpn) {
            struct framephy_struct *ret = fp->fp_next;
            fp->fp_next = fp->fp_next->fp_next;
            ret->fp_next = NULL;
            return 0;
         }
         else fp = fp->fp_next;
      }
      return -1;
   }
}

int MEMPHY_put_usedfp(struct memphy_struct *mp, int fpn, struct mm_struct *owner)
{
   struct framephy_struct *fp = mp->used_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->owner = owner;
   newnode->fp_next = NULL;
   /* Push new frame to the end of the list*/
   if (!fp) {
      mp->used_fp_list = newnode;
   }
   else {
      while (fp->fp_next) {
         fp = fp->fp_next;
      } 
      fp->fp_next = newnode;
   }
   return 0;
}

struct framephy_struct* MEMPHY_get_usedfp(struct memphy_struct *mp)
{
   struct framephy_struct *fp = mp->used_fp_list;

   if (fp == NULL)
     return NULL;
   mp->used_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   return fp;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;
   mp->used_fp_list = NULL;
   MEMPHY_format(mp,PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

#endif

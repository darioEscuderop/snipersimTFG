#ifndef TLB_H
#define TLB_H

#include "fixed_types.h"
#include "cache.h"
#include "page_table.h"
#include "page_stats.h"
#include "hot_super_page_struct.h"

namespace ParametricDramDirectoryMSI
{
   class PageTable;
   
   class TLB
   {
      private:
         UInt32 m_size;
         UInt32 m_associativity;
         Cache m_cache;

         TLB *m_next_level;

         UInt64 m_access, m_miss;

         TLB *m_2mb_tlb;

         PageTable *m_page_table;

         PageStats *m_page_stats;

         IntPtr m_block_size;

         HotSuperPageStruct *m_hot_super_page_struct;

      public:
         TLB(String name, String cfgname, core_id_t core_id, UInt32 num_entries, UInt32 associativity, TLB *next_level, TLB * tlb_2mb, PageTable *pT, PageStats *pageStats, IntPtr block_size);
         bool lookup(IntPtr address, SubsecondTime now, bool allocate_on_miss = true);
         void allocate(IntPtr address, SubsecondTime now);
         void evictPageIfPresent(IntPtr address);

         static const UInt32 SIM_PAGE_SHIFT = 12; // 4KB
         static const IntPtr SIM_PAGE_SIZE = (1L << SIM_PAGE_SHIFT);
         static const IntPtr SIM_PAGE_MASK = ~(SIM_PAGE_SIZE - 1);

         static const UInt32 SIM_PAGE_SHIFT_2MB = 21; // 2MB
         static const IntPtr SIM_PAGE_SIZE_2MB = (1L << SIM_PAGE_SHIFT_2MB);
         static const IntPtr SIM_PAGE_MASK_2MB = ~(SIM_PAGE_SIZE_2MB - 1);


   };
}

#endif // TLB_H

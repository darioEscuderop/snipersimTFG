#include "tlb.h"
#include "stats.h"
#include "simulator.h"
#include "config.hpp"
namespace ParametricDramDirectoryMSI
{
//4kb --> 2mb
TLB::TLB(String name, String cfgname, core_id_t core_id, UInt32 num_entries, UInt32 associativity, TLB *next_level,  TLB * tlb_2mb, PageTable *pT, PageStats *pageStats, IntPtr blocksize)
   : m_size(num_entries)
   , m_associativity(associativity)
   , m_cache(name + "_cache", cfgname, core_id, num_entries / associativity, associativity,  blocksize, "lru", CacheBase::PR_L1_CACHE)
   , m_next_level(next_level)
   , m_access(0)
   , m_miss(0)
   , m_2mb_tlb(tlb_2mb)
   , m_page_table(pT)
   , m_page_stats(pageStats)
   , m_block_size(blocksize)
   
    
{
   LOG_ASSERT_ERROR((num_entries / associativity) * associativity == num_entries, "Invalid TLB configuration: num_entries(%d) must be a multiple of the associativity(%d)", num_entries, associativity);
   registerStatsMetric(name, core_id, "access", &m_access);
   registerStatsMetric(name, core_id, "miss", &m_miss);
   //bool ejecucion = Sim()->getCfg()->getBool("tfg/dario/conteo_uso_paginas");
   //pageStats = PageStats(ejecucion, core_id);
}

bool
TLB::lookup(IntPtr address, SubsecondTime now, bool allocate_on_miss)
{
   bool hit = false;
    hit = m_cache.accessSingleLine(address, Cache::LOAD, NULL, 0, now, true);

    if(m_next_level != NULL){
      m_page_stats->updatePageStats(address, m_block_size, false);
    }
   m_access++;
   if (hit){
      return true;
   }
   m_miss++; //4kb miss o 2mb_miss
   if(hit)
      return true;
   
   //check HotSuperPageStruct
   if(m_hot_super_page_struct && m_hot_super_page_struct->existsByBloomFilter(address)){
      return true;
   }


   //check stlb
   if (m_next_level)
   {
      hit = m_next_level->lookup(address, now, false /* no allocation */);
   }

   if (allocate_on_miss)
   {
      //Aqui en este punto venimos de un fallo de la L1TLB se inserta en la TLB
      allocate(address, now);
   }

   if(!hit){
      if(m_next_level != NULL ){ // LO hace la TLB de 2MB o 4KB
         m_page_table->insertPageIfNotPresent(address, m_block_size);
      }

   }
   return hit;
}


void
TLB::allocate(IntPtr address, SubsecondTime now)
{
   bool eviction;
   IntPtr evict_addr;
   CacheBlockInfo evict_block_info;
   m_cache.insertSingleLine(address, NULL, &eviction, &evict_addr, &evict_block_info, NULL, now);

   // Use next level as a victim cache
   if (eviction && m_next_level){
  //    pageStats.updatePageStats(address, true);
      m_next_level->allocate(evict_addr, now);
   }
}

void
TLB::evictPageIfPresent(IntPtr address)
{
   m_cache.invalidateSingleLine(address);
   if(m_page_stats){
      m_page_stats->evictPage(address, m_block_size);
     // std::cout << "Evicting: " << std::hex << address << std::endl;
   }

   if(m_hot_super_page_struct){
      m_hot_super_page_struct->remove(address);
   }

}

}

#include "tlb.h"
#include "stats.h"

namespace ParametricDramDirectoryMSI
{

TLB::TLB(String name, String cfgname, core_id_t core_id, UInt32 num_entries, UInt32 associativity, TLB *next_level)
   : m_size(num_entries)
   , m_associativity(associativity)
   , m_cache(name + "_cache", cfgname, core_id, num_entries / associativity, associativity, SIM_PAGE_SIZE, "lru", CacheBase::PR_L1_CACHE)
   , m_next_level(next_level)
   , m_access(0)
   , m_miss(0)
{
   LOG_ASSERT_ERROR((num_entries / associativity) * associativity == num_entries, "Invalid TLB configuration: num_entries(%d) must be a multiple of the associativity(%d)", num_entries, associativity);

   registerStatsMetric(name, core_id, "access", &m_access);
   registerStatsMetric(name, core_id, "miss", &m_miss);
   pageStats = PageStats(Sim()->getCfg()->getBool("tfg/dario/conteo_uso_paginas"), core_id);
}

bool
TLB::lookup(IntPtr address, SubsecondTime now, bool allocate_on_miss)
{
   bool hit = m_cache.accessSingleLine(address, Cache::LOAD, NULL, 0, now, true);

   m_access++;
   //De momento solo buscamos el saber cuantas veces se accede a una pagina y a sus words para sacar un promedio
   //Si está en la configuración hacer, si no, no hacer nada

   pageStats.updatePageStats(address);

   if (hit){
      return true;
   }

   m_miss++;

   if (m_next_level)
   {
      hit = m_next_level->lookup(address, now, false /* no allocation */);
   }

   if (allocate_on_miss)
   {
      allocate(address, now);
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

}

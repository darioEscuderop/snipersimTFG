#ifndef PAGE_STATS_H
#define PAGE_STATS_H

#include "fixed_types.h"
#include <unordered_map>
#include <bitset>
#include <set>

class PageStats {
public:
    PageStats(bool ejecucion, core_id_t core_id);
    void updatePageStats(IntPtr pageAdress, bool evict = false);
    ~PageStats();
private:
    std::unordered_map<IntPtr, std::bitset<1024>> pageMap; // Asumimos que el tamano de la pagina es 4096 bytes (1024 palabras de 4 bytes)
    std::unordered_map<IntPtr, UInt64> pageAccessCount;
    UInt64 totalPages;
    UInt64 minUsage;
    UInt64 maxUsage;
    UInt64 totalAccesses;
    UInt64 totalUse;
    core_id_t core;
    bool ejecucion;
    void evictPage(IntPtr pageAddress);
    std::set<int> countedPages;

};

#endif // PAGE_STATS_H
#ifndef PAGE_STATS_H
#define PAGE_STATS_H

#include "fixed_types.h"
#include <unordered_map>
#include <bitset>
#include <set>

class PageStats {
public:
    PageStats(bool ejecucion, core_id_t core_id, int tipo);
    void updatePageStats(IntPtr address, IntPtr page_size, bool evict = false);
    void evictPage(IntPtr pageAddress, IntPtr page_size);
    ~PageStats();
private:
    std::unordered_map<IntPtr, std::bitset<(1L<<21 )/4>> pageMap; // Asumimos que el tamano de la pagina es 4096 bytes (1024 palabras de 4 bytes)
    std::unordered_map<IntPtr, UInt64> pageAccessCount;
    UInt64 totalPages;
    UInt64 minUsage;
    UInt64 maxUsage;
    UInt64 totalAccesses;
    UInt64 totalUse;
    core_id_t core;
    bool ejecucion;
    int tipo;
    std::set<int> countedPages;

};

#endif // PAGE_STATS_H
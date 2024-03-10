#ifndef PAGE_STATS_H
#define PAGE_STATS_H

#include "fixed_types.h"
#include <unordered_map>
#include <bitset>

class PageStats {
public:
    PageStats(bool ejecucion, core_id_t core_id);
    void updatePageStats(int pageAdress, bool evict = false);

private:
    std::unordered_map<int, std::bitset<1024>> pageMap; // Asumimos que el tamano de la pagina es 4096 bytes (1024 palabras de 4 bytes)
    std::unordered_map<int, int> pageAccessCount;
    int totalPages;
    int minUsage;
    int maxUsage;
    long long int totalAccesses;
    long long int totalUse;
    bool ejecucion;
    void evictPage(int pageAddress);
};

#endif // PAGE_STATS_H
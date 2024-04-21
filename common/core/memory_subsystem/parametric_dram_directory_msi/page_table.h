#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "fixed_types.h"
#include <map>
#include <list>
#include <bitset>
#include "page.h"
#include <set>

 enum TipoSistemaMemoria {
        PAGINAS_4KB = 1,
        PAGINAS_2MB =2 ,
        PAGINAS_4KB_2MB =3 ,
        PAGINAS_2MB_SISTEMA_MEMORIA_PAGINAS_CALIENTES = 4
    };

class PageTable {
public:

    static const UInt32 SIM_PAGE_SHIFT_4KB = 12; // 4KB
    static const IntPtr SIM_PAGE_SIZE_4KB = (1L << SIM_PAGE_SHIFT_4KB);
    static const IntPtr SIM_PAGE_MASK_4KB = ~(SIM_PAGE_SIZE_4KB - 1);

    static const UInt32 SIM_PAGE_SHIFT_2MB = 21; // 2MB
    static const IntPtr SIM_PAGE_SIZE_2MB = (1L << SIM_PAGE_SHIFT_2MB);
    static const IntPtr SIM_PAGE_MASK_2MB = ~(SIM_PAGE_SIZE_2MB - 1);

    static const UInt32 WORD_SIZE = 4; // 4 bytes

    core_id_t core_id;

    std::map<IntPtr, Page*> page_table_4kb;
    std::map<IntPtr, Page*> page_table_2mb;
    std::list<IntPtr> lru_4kb;
    std::list<IntPtr> lru_2mb;
    UInt64 memoria;

    struct RecordMemoria {
        std::map<IntPtr, std::bitset<1024>> pageMap;
        std::map<IntPtr, UInt64> pageAccessCount;
        std::set<int> countedPages;
        UInt64 totalPages;
        UInt64 minUsage;
        UInt64 maxUsage;
        UInt64 totalAccesses;
        UInt64 totalUse;
    } recordMemoria;

    int tipoSistema;

    PageTable(UInt64 memoria, core_id_t core_id);
    Page* insertPage(IntPtr address, IntPtr page_size);

    Page* insertPageIfNotPresent(IntPtr address);

private: 
    Page* insertSinglePage(IntPtr address, IntPtr page_size);
    Page* updatePage(IntPtr address, IntPtr page_size);
    void removePage(IntPtr address, IntPtr page_size);
    void removeLastPage(IntPtr page_size);
    void calculo(UInt64 vp);
};


#endif // PAGE_TABLE_H
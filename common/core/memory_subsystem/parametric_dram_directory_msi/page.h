#ifndef PAGE_H
#define PAGE_H

#include "fixed_types.h"
#include <list>

enum TipoPage {
        PAGINA_NORMAL = 1,
        PAGINA_CALIENTE = 2
};

class Page{
    public:
        Page(IntPtr address, IntPtr page_size, TipoPage tipo);
        IntPtr address;
        IntPtr page_size;
        //Posición de la lista de la LRU
        std::list<IntPtr>::iterator lru_it;
        bool operator==(const Page &p) const;
        bool operator<(const Page &p) const;

        TipoPage tipo;
};

#endif // PAGE_H
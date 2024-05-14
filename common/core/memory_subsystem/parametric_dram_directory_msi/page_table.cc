#include "page_table.h"
#include "simulator.h"
#include "stats.h"
#include "config.hpp"
#include <climits>

namespace ParametricDramDirectoryMSI{

PageTable::PageTable (UInt64 memoria, core_id_t coreId) {
    this->memoria = memoria;
    this->core_id = coreId;
    this->recordMemoria.totalPages = 0;
    this->recordMemoria.minUsage = 0;
    this->recordMemoria.maxUsage = 0;
    this->recordMemoria.totalAccesses = 0;
    this->recordMemoria.totalUse = 0;
    this->tipoSistema = Sim()->getCfg()->getInt("tfg/dario/tipo_sistema_memoria");

   recordMemoria.maxUsage = 0;
   recordMemoria.minUsage = ULONG_MAX;
   registerStatsMetric("tfg_dario", coreId, "total_page_table_walk", &recordMemoria.totalPages);
 //  registerStatsMetric("tfg_dario", coreId, "min_usage", &recordMemoria.minUsage);
 //  registerStatsMetric("tfg_dario", coreId, "max_usage", &recordMemoria.maxUsage);
  // registerStatsMetric("tfg_dario", coreId, "total_use", &recordMemoria.totalUse);
  // registerStatsMetric("tfg_dario", coreId, "total_accesses", &recordMemoria.totalAccesses);
}


Page* PageTable::insertSinglePage(IntPtr vp, IntPtr page_size){
    Page* page = NULL;
    if (page_size == SIM_PAGE_SIZE_4KB) {
        // Insertarla y actualizar el LRU
        std::list<IntPtr>::iterator it = lru_4kb.insert(lru_4kb.begin(), vp);
        page = new Page(vp, page_size, PAGINA_NORMAL);
        page->lru_it = it; // Guardar el iterador en el objeto de la página
        page_table_4kb[vp] = page;
    } else if (page_size == SIM_PAGE_SIZE_2MB) {
        std::list<IntPtr>::iterator it = lru_2mb.insert(lru_2mb.begin(), vp);
        page = new Page(vp, page_size, PAGINA_NORMAL);
        page->lru_it = it; // Guardar el iterador en el objeto de la página
        page_table_2mb[vp] = page;
    }
    return page;
}

Page* PageTable::insertPage (IntPtr address, IntPtr page_size) {

    uintptr_t vp = address / page_size;
    //uintptr_t full_page = address & ~(page_size - 1);
    //Comprobar si ya está y si está actualizar la LRU
    Page* page = updatePage(vp, page_size);
    if(page != NULL){
        return page;
    }
    double memory_usage = (this->page_table_2mb.size() * SIM_PAGE_SIZE_2MB + this->page_table_4kb.size() * SIM_PAGE_SIZE_4KB) / static_cast<double>(this->memoria);
   // std::cout << "Memory usage: " << memory_usage << std::endl;
   // std::cout << "VP: " << "0x" << std::hex << full_page << std::endl;
    if(memory_usage >= 1){
        if(page_size == SIM_PAGE_SIZE_4KB){
            removeLastPage(SIM_PAGE_SIZE_4KB);
        }else if(page_size == SIM_PAGE_SIZE_2MB){
            removeLastPage(SIM_PAGE_SIZE_2MB);
        }
    }

    return insertSinglePage(vp, page_size);
    
}

Page* PageTable::updatePage (IntPtr address, IntPtr page_size) {
    Page *page = NULL;
    if (page_size == 4096) {
        if (this->page_table_4kb.find(address) != this->page_table_4kb.end()) {
            this->lru_4kb.erase(this->page_table_4kb[address]->lru_it);
            this->page_table_4kb[address]->lru_it = this->lru_4kb.insert(this->lru_4kb.end(), address);
            page =this->page_table_4kb[address];
        }
    } else if (page_size == 2097152) {
        if (this->page_table_2mb.find(address) != this->page_table_2mb.end()) {
            this->lru_2mb.erase(this->page_table_2mb[address]->lru_it);
            this->page_table_2mb[address]->lru_it = this->lru_4kb.insert(this->lru_2mb.end(), address);
            page = this->page_table_2mb[address];
        }
    }
    return page;
}

void PageTable::removeLastPage(IntPtr page_size){
    if (page_size == SIM_PAGE_SIZE_4KB) {
        if (!this->lru_4kb.empty()) {
            IntPtr address = this->lru_4kb.back();
            this->lru_4kb.pop_back();
            this->page_table_4kb.erase(address);
            evictPageTLBs(address);
        }
    } else if (page_size == SIM_PAGE_SIZE_2MB) {
        if (!this->lru_2mb.empty()) {
            IntPtr address = this->lru_2mb.back();
            this->lru_2mb.pop_back();
            this->page_table_2mb.erase(address);
            evictPageTLBs(address);
        }
    }
}

void PageTable::evictPageTLBs(IntPtr address){
    for(auto tlb : tlbs){
        tlb->evictPageIfPresent(address);
    }
}


void PageTable::removePage (IntPtr address, IntPtr page_size) {
    if (page_size == 4096) {
        if (this->page_table_4kb.find(address) != this->page_table_4kb.end()) {
            this->lru_4kb.erase(this->page_table_4kb[address]->lru_it);
            this->page_table_4kb.erase(address);
            evictPageTLBs(address);
        }
    } else if (page_size == 2097152) {
        if (this->page_table_2mb.find(address) != this->page_table_2mb.end()) {
            this->lru_2mb.erase(this->page_table_2mb[address]->lru_it);
            this->page_table_2mb.erase(address);
            evictPageTLBs(address);
        }
    }
}

void PageTable::calculo(UInt64 vp){
      // Convertir la dirección de la página en un número de página
     /* int page = vp / page_size; 

      // Convertir la dirección de la página en un índice de palabra dentro de la página
      int word = (vp % page_size) / WORD_SIZE; // Asume que el tamaño de la palabra es de 4 bytes

      // Actualizar el mapa de la página
      if(recordMemoria.pageMap[page][word] == 0){
         recordMemoria.pageMap[page][word] = 1;
         recordMemoria.totalUse +=1; 
      }

      // Verificar si la página ha sido accedida anteriormente
      if (recordMemoria.countedPages.find(page) == recordMemoria.countedPages.end()){
         // Incrementar el contador de páginas accedidas
         recordMemoria.countedPages.insert(page);
         recordMemoria.totalPages++;
      } 

      // Actualizar el contador de acceso a la página
      recordMemoria.pageAccessCount[page]++;
      recordMemoria.totalAccesses++;

      recordMemoria.minUsage = std::min(recordMemoria.minUsage, recordMemoria.pageAccessCount[page]);
      recordMemoria.maxUsage = std::max(recordMemoria.maxUsage, recordMemoria.pageAccessCount[page]);*/
}


Page* PageTable::insertPageIfNotPresent (IntPtr address, IntPtr block_size) {
    Page* page = NULL;
    //calculo(address);
    recordMemoria.totalPages++;
    switch(tipoSistema){
        case PAGINAS_4KB:
            page = insertPage(address, block_size);
            break;
        case PAGINAS_2MB:
            page = insertPage(address, block_size);
            break;
        case PAGINAS_4KB_2MB:
            insertPage(address, SIM_PAGE_SIZE_4KB);
            insertPage(address, SIM_PAGE_SIZE_2MB);
            break;
        case PAGINAS_2MB_SISTEMA_MEMORIA_PAGINAS_CALIENTES:
            break;   
    }

    return page;
}


void PageTable::addTLB(TLB* tlb){
    tlbs.push_back(tlb);
}
}
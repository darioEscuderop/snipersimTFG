#include "page_stats.h"
#include <algorithm>
#include <climits>
#include "stats.h"
#include "simulator.h"

#define PAGE_SIZE 4096
#define WORD_SIZE 4

PageStats::PageStats(bool ejec, core_id_t core_id) : totalPages(0), minUsage(0), maxUsage(0) {
    // Inicializar todos los elementos del mapa de páginas y el contador de acceso a la página a 0
    for (auto& pair : pageMap) {
        pair.second.reset();
    }
    for (auto& pair : pageAccessCount) {
        pair.second = 0;
    }
    ejecucion = ejec;
    totalUse = 0;
    totalAccesses = 0;
    core = core_id;
    countedPages = std::set<int>();

    registerStatsMetric("tfg_dario", core, "total_pages", &totalPages);
    registerStatsMetric("tfg_dario", core, "min_usage", &minUsage);
    registerStatsMetric("tfg_dario", core, "max_usage", &maxUsage);
    registerStatsMetric("tfg_dario", core, "total_use", &totalUse);
    registerStatsMetric("tfg_dario", core, "total_accesses", &totalAccesses);
}


PageStats::~PageStats(){
  
}

void PageStats::updatePageStats(IntPtr pageAddress, bool evictedPage) {
    if(!ejecucion) return;
    
    if(evictedPage){
        evictPage(pageAddress);
    }
    // Convertir la dirección de la página en un número de página
    int page = pageAddress / PAGE_SIZE; 

    // Convertir la dirección de la página en un índice de palabra dentro de la página
    int word = (pageAddress % PAGE_SIZE) / WORD_SIZE; // Asume que el tamaño de la palabra es de 4 bytes

    // Actualizar el mapa de la página
    if(pageMap[page][word] == 0){
        pageMap[page][word] = 1;
        totalUse +=1; 
    }

    // Verificar si la página ha sido accedida anteriormente
    if (countedPages.find(page) == countedPages.end()){
        // Incrementar el contador de páginas accedidas
        countedPages.insert(page);
        totalPages++;
    } 

    // Actualizar el contador de acceso a la página
    pageAccessCount[page]++;
    totalAccesses++;

    minUsage = std::min(minUsage, pageAccessCount[page]);
    maxUsage = std::max(maxUsage, pageAccessCount[page]);
    
}

void PageStats::evictPage(IntPtr pageAddress) {
    // Convertir la dirección de la página en un número de página
    int page = pageAddress / PAGE_SIZE;

    countedPages.erase(page);
    // Eliminar la entrada correspondiente a la página en pageMap y pageAccessCount
    pageMap[page].reset();
    pageAccessCount[page] = 0;

}
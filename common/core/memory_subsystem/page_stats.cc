#include "page_stats.h"
#include <algorithm>
#include <climits>
#include "stats.h"
#include "simulator.h"

#define PAGE_SIZE 4096
#define WORD_SIZE 4

PageStats::PageStats(bool ejec, core_id_t core_id, int tipo) : 
    totalPages(0), 
    minUsage(0), 
    maxUsage(0), 
    totalAccesses(0), 
    totalUse(0), 
    core(core_id), 
    ejecucion(ejec),
    tipo(tipo)
{
    // Inicializar todos los elementos del mapa de páginas y el contador de acceso a la página a 0
    for (auto& pair : pageMap) {
        pair.second.reset();
    }
    for (auto& pair : pageAccessCount) {
        pair.second = 0;
    }
    countedPages = std::set<int>();


    registerStatsMetric("tfg_dario", core_id, "total_pages", &totalPages);
    registerStatsMetric("tfg_dario", core_id, "min_usage", &minUsage);
    registerStatsMetric("tfg_dario", core_id, "max_usage", &maxUsage);
    registerStatsMetric("tfg_dario", core_id, "total_use", &totalUse);
    registerStatsMetric("tfg_dario", core_id, "total_accesses", &totalAccesses);
    
}


PageStats::~PageStats(){
   
    
}

void PageStats::updatePageStats(IntPtr address, IntPtr page_size, bool evictedPage) {
    if(!ejecucion) return;

    if(evictedPage){
        evictPage(address, page_size);
    }
    // Convertir la dirección de la página en un número de página
    int page = address / page_size; 

    // Convertir la dirección de la página en un índice de palabra dentro de la página
    int word = (address % page_size) / WORD_SIZE; // Asume que el tamaño de la palabra es de 4 bytes

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


 /*   
    std::cout <<"Total Pages" << totalPages << std::endl;
    std::cout <<"Min Usage" << minUsage << std::endl;
    std::cout <<"Max Usage" << maxUsage << std::endl;
    std::cout <<"Total Use" << totalUse << std::endl;
    std::cout <<"Total Accesses" << totalAccesses << std::endl;
   */
}

void PageStats::evictPage(IntPtr address, IntPtr page_size) {
    // Convertir la dirección de la página en un número de página
    int page = address / page_size;

    // Verificar si la página existe en el mapa de páginas
    if (pageMap.find(page) == pageMap.end()) {
        return; // La página no existe, no hacer nada
    }

    countedPages.erase(page);
    // Eliminar la entrada correspondiente a la página en pageMap y pageAccessCount
    pageMap[page].reset();
    pageAccessCount[page] = 0;

}
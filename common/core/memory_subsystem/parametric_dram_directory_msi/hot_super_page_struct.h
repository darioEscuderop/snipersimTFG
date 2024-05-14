#pragma once
#include "fixed_types.h"
#include <list>
#include "bloom_filter.h"

namespace ParametricDramDirectoryMSI{
    static const UInt32 MAX_LIST_SIZE = 512; // 2MB de paginas de 4KB = 512 paginas
    static const UInt32 NUM_HASHES = 3;

    class HotPage;
    class HotSuperPage;
    class HotSuperPageStruct;
    //Aqui definiremos un HotSuperPageStruct, la cual tendrá una nueva estructura de HotSuperPage que será una lista a HotPages que se encuentran en la misma super página.
    //Además, tendrá un contador de cuántas veces se ha accedido a la super página. 
    //Además de ello tendremos un BloomFilter que dado un address nos dirá si la página se encuentra en la super página de una HotSuperPage de las diferentes que tiene
    class HotPage{
        private:
            IntPtr address;
            UInt32 access_count;
        public:
            HotPage(IntPtr address, UInt32 access_count);
            ~HotPage();
            IntPtr getAddress();
            UInt32 getAccessCount();
            void incrementAccessCount();
    };

class HotSuperPage{
    private:
        UInt32 access_count;
        BloomFilter *bloom_filter;
    public:
        std::list<HotPage> hot_pages;
        HotSuperPage();
        ~HotSuperPage();
        void insert(IntPtr address, UInt32 access_count);
        void remove(IntPtr address);
        void update(IntPtr address);
        bool exists(IntPtr address);
        bool existsByBloomFilter(IntPtr address);
        UInt32 getAccessCount();
        void incrementAccessCount();
        // Function to sort the hot_pages list based on access_count
        void sortHotPages();
    };



    class HotSuperPageStruct
    {
    private:
            UInt32 num_hot_pages;
            UInt32 bloom_filter_size;
            std::list<HotSuperPage> hot_super_pages;
            UInt32 access_count;
    public:
        //Constructor de HotSuperPageStruct
        HotSuperPageStruct(UInt32 num_hot_pages, UInt32 bloom_filter_size);
        //Destructor de HotSuperPageStruct
        ~HotSuperPageStruct();
        bool exists(IntPtr address);
        bool existsByBloomFilter(IntPtr address);
        void insert(IntPtr address, UInt32 access_count);
        void remove(IntPtr address);
        void update(IntPtr address);
        void incrementAccessCount();
        void replaceHotSuperPage(UInt32 access_count, IntPtr address);
        // Function to sort the hot_super_pages list based on access_count
        void sortSuperPages();
        UInt32 getAccessCount();
    };
}

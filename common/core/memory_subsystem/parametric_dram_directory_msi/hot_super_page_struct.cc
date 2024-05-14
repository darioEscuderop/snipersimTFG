#include "hot_super_page_struct.h"
#include "simulator.h"
#include "config.hpp"


namespace ParametricDramDirectoryMSI
{
    HotPage::HotPage(IntPtr address, UInt32 access_count)
    {
        this->address = address;
        this->access_count = access_count;
    }

    HotPage::~HotPage()
    {
    }

    IntPtr HotPage::getAddress()
    {
        return address;
    }

    UInt32 HotPage::getAccessCount()
    {
        return access_count;
    }

    void HotPage::incrementAccessCount()
    {
        access_count++;
    }

    HotSuperPage::HotSuperPage()
    {
        access_count = 0;
        bloom_filter = new BloomFilter(MAX_LIST_SIZE, NUM_HASHES);
    }

    HotSuperPage::~HotSuperPage()
    {

    }

    bool HotSuperPage::existsByBloomFilter(IntPtr address)
    {
        return bloom_filter->contains(std::to_string(address));
    }

    void HotSuperPage::insert(IntPtr address, UInt32 access_count)
    {
        // Check if the address already exists in the hot pages list
        for (std::list<HotPage>::iterator it = hot_pages.begin(); it != hot_pages.end(); it++)
        {
            if (it->getAddress() == address)
            {
                // Update the access count of the existing hot page
                it->incrementAccessCount();
                //sortHotPages();
                return;
            }
        }

        // If the address doesn't exist, create a new hot page and insert it into the list
        hot_pages.push_back(HotPage(address, access_count));
        //sortHotPages();
    }

    void HotSuperPage::remove(IntPtr address)
    {
        for (std::list<HotPage>::iterator it = hot_pages.begin(); it != hot_pages.end(); it++)
        {
            if (it->getAddress() == address)
            {
                hot_pages.erase(it);
                //sortHotPages();
                break;
            }
        }
    }

    void HotSuperPage::update(IntPtr address)
    {
        for (std::list<HotPage>::iterator it = hot_pages.begin(); it != hot_pages.end(); it++)
        {
            if (it->getAddress() == address)
            {
                it->incrementAccessCount();
                //sortHotPages();
                break;
            }
        }
    }

    bool HotSuperPage::exists(IntPtr address)
    {
        for (std::list<HotPage>::iterator it = hot_pages.begin(); it != hot_pages.end(); it++)
        {
            if (it->getAddress() == address)
            {
                return true;
            }
        }
        return false;
    }

    UInt32 HotSuperPage::getAccessCount()
    {
        return access_count;
    }

    void HotSuperPage::incrementAccessCount()
    {
        access_count++;
    }

    HotSuperPageStruct::HotSuperPageStruct(UInt32 num_hot_pages, UInt32 bloom_filter_size)
    {
        this->num_hot_pages = Sim()->getCfg()->getInt("tfg/dario/num_hot_pages");
        this->bloom_filter_size = bloom_filter_size;
        this->access_count = 0;
    }

    HotSuperPageStruct::~HotSuperPageStruct()
    {
    }

    bool HotSuperPageStruct::exists(IntPtr address)
    {
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->exists(address))
            {
                return true;
            }
        }
        return false;
    }

    void HotSuperPageStruct::insert(IntPtr address, UInt32 access_count)
    {
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->exists(address))
            {
                it->update(address);
                //sortSuperPages();
                return;
            }
        }

        //A la hora de insertar vemos si hay alguna HotSuperPage que tenga menos de num_hot_pages, si la hay insertamos en esa HotSuperPage y si no creamos una nueva HotSuperPage
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->hot_pages.size() < MAX_LIST_SIZE)
            {
                it->insert(address, access_count);
                //sortSuperPages();
                return;
            }
        }
        // Si no hemos alcanzado el número máximo de HotPages, creamos una nueva HotSuperPage
        if (hot_super_pages.size() < num_hot_pages)
        {
            HotSuperPage new_hot_super_page;
            new_hot_super_page.insert(address, access_count);
            hot_super_pages.push_back(new_hot_super_page);
        }else{
            //Se procede a sustituir de la HotSuperPage con menos accesos a la HotSuperPage con menos accesos hazlo desde una función aparte
            replaceHotSuperPage(access_count, address);
            
        }
    }

    void HotSuperPageStruct::replaceHotSuperPage(UInt32 access_count, IntPtr address){
        //Se busca la HotSuperPage con menos accesos
        //Se elimina la HotPage con menos accesos de la SuperPage
        //Se inserta la nueva HotPage en la SuperPage
        UInt32 min_access_count = hot_super_pages.front().getAccessCount();
        std::list<HotSuperPage>::iterator min_access_count_it = hot_super_pages.begin();
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->getAccessCount() < min_access_count)
            {
                min_access_count = it->getAccessCount();
                min_access_count_it = it;
            }
        }
        min_access_count_it->remove(min_access_count_it->hot_pages.front().getAddress());
        min_access_count_it->insert(address, access_count);
    }

    void HotSuperPageStruct::remove(IntPtr address)
    {
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->exists(address))
            {
                it->remove(address);
                //sortSuperPages();
                break;
            }
        }
    }

    void HotSuperPageStruct::update(IntPtr address)
    {
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++)
        {
            if (it->exists(address))
            {
                it->update(address);
                //sortSuperPages();
                break;
            }
        }
    }

    void HotSuperPageStruct::incrementAccessCount()
    {
        access_count++;
    }

    UInt32 HotSuperPageStruct::getAccessCount()
    {
        return access_count;
    }


    bool HotSuperPageStruct::existsByBloomFilter(IntPtr address){
        //Recorremos cada uno con bloomFilter y si existe recorremos ese mismo
        for (std::list<HotSuperPage>::iterator it = hot_super_pages.begin(); it != hot_super_pages.end(); it++){
            if(it->existsByBloomFilter(address)){
                if(it->exists(address))
                    return true;
            }
        }
        return false;
    }
} // namespace ParametricDramDirectoryMSI
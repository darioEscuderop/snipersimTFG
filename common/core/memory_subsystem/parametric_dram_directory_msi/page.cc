#include "page.h"

Page::Page(IntPtr address, IntPtr page_size, TipoPage tp) : 
    address(address), 
    page_size(page_size), 
    tipo(tp)
{

}

bool Page::operator==(const Page &p) const {
    return address == p.address && page_size == p.page_size;
}

bool Page::operator<(const Page &p) const {
    if (address < p.address) {
        return true;
    } else if (address == p.address) {
        return page_size < p.page_size;
    } else {
        return false;
    }
}
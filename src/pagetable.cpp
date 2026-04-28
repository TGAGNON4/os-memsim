#include <algorithm>
#include "pagetable.h"

PageTable::PageTable(int page_size)
{
    _page_size = page_size;
}

PageTable::~PageTable()
{
}

std::vector<std::string> PageTable::sortedKeys()
{
    std::vector<std::string> keys;

    std::map<std::string, int>::iterator it;
    for (it = _table.begin(); it != _table.end(); it++)
    {
        keys.push_back(it->first);
    }

    std::sort(keys.begin(), keys.end(), PageTableKeyComparator());

    return keys;
}

void PageTable::addEntry(uint32_t pid, int page_number)
{
    // Combination of pid and page number act as the key to look up frame number
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);

    int frame = 0; 
    // Find free frame
    bool found = false;
    while(!found){
        found = true; // in case _table is empty or found a free frame
        std::map<std::string, int>::iterator it;
        for (it = _table.begin(); it != _table.end(); it++)
        {
            // if that frame is being used then skip
            if(it->second == frame){
                found = false;
                frame++;
                break; // restart scan with new frame value
            }
        }
    }

    _table[entry] = frame;
}

bool PageTable::hasEntry(uint32_t pid, int page_number)
{
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);
    return _table.count(entry) > 0;
}

void PageTable::removeEntry(uint32_t pid, int page_number)
{
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);
    _table.erase(entry);
}

void PageTable::removeEntries(uint32_t pid)
{
    std::vector<std::string> keys = sortedKeys();

    for (size_t i = 0; i < keys.size(); i++)
    {
        int delim = keys[i].find('|');
        uint32_t entry_pid = std::stoi(keys[i].substr(0, delim));

        if (entry_pid == pid)
        {
            _table.erase(keys[i]);
        }
    }
}

int PageTable::getPhysicalAddress(uint32_t pid, uint32_t virtual_address)
{
    // Convert virtual address to page_number and page_offset
    int page_number = virtual_address / _page_size;
    int page_offset = virtual_address % _page_size;

    // Combination of pid and page number act as the key to look up frame number
    std::string entry = std::to_string(pid) + "|" + std::to_string(page_number);
    
    // If entry exists, look up frame number and convert virtual to physical address
    int address = -1;
    if (_table.count(entry) > 0)
    {
        address = _table[entry] * _page_size + page_offset;
    }

    return address;
}

uint32_t PageTable::getEntryCount()
{
    return _table.size();
}

void PageTable::print()
{
    int i;

    std::cout << " PID  | Page Number | Frame Number" << std::endl;
    std::cout << "------+-------------+--------------" << std::endl;

    std::vector<std::string> keys = sortedKeys();

    for (i = 0; i < keys.size(); i++)
    {
        int delim = keys[i].find('|');
        uint32_t pid = std::stoi(keys[i].substr(0, delim));
        uint32_t page_n = std::stoi(keys[i].substr(delim + 1));
        uint32_t frame_n = _table[keys[i]];
        printf(" %4d | %11d | %12d\n",
            pid,
            page_n,
            frame_n);
    }
}

uint32_t PageTable::getPageSize(){
    return _page_size;
}

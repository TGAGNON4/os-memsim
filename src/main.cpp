#include <iostream>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "mmu.h"
#include "pagetable.h"

// 64 MB (64 * 1024 * 1024)
#define PHYSICAL_MEMORY 67108864

bool isValidPageSize(int page_size);
std::vector<std::string> splitCommand(const std::string &command);
uint32_t getTypeSize(DataType type);
std::vector<uint8_t> readBytes(uint32_t pid, uint32_t virtual_address, uint32_t size, PageTable *page_table, uint8_t *memory);
void printStartMessage(int page_size);
void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table);
void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table);
void setVariable(uint32_t pid, std::string var_name, uint32_t offset, std::string value, Mmu *mmu, PageTable *page_table, uint8_t *memory);
void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table);
void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table);
void printProcesses(Mmu *mmu);
void printVariableValue(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table, uint8_t *memory);

int main(int argc, char **argv)
{
    // Ensure user specified page size as a command line parameter
    if (argc < 2)
    {
        std::cerr << "Error: you must specify the page size" << std::endl;
        return 1;
    }

    // Print opening instuction message
    int page_size = std::stoi(argv[1]);
    if (!isValidPageSize(page_size))
    {
        std::cerr << "Error: page size must be a power of 2 between 1024 and 32768" << std::endl;
        return 1;
    }
    printStartMessage(page_size);

    // Create physical 'memory' (raw array of bytes)
    uint8_t *memory = new uint8_t[PHYSICAL_MEMORY]();

    // Create MMU and Page Table
    Mmu *mmu = new Mmu(PHYSICAL_MEMORY);
    PageTable *page_table = new PageTable(page_size);

    // Prompt loop
    std::string command;
    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, command))
        {
            break;
        }

        std::vector<std::string> tokens = splitCommand(command);
        if (tokens.empty())
        {
            continue;
        }

        std::string cmd = tokens[0];
        if (cmd == "exit")
        {
            break;
        }

        // Handle command
        if(cmd == "create" && tokens.size() >= 3){
            int text_size = std::stoi(tokens[1]);
            int data_size = std::stoi(tokens[2]);
            createProcess(text_size, data_size, mmu, page_table);
        }
        else if(cmd == "allocate" && tokens.size() >= 5){
            int pid = std::stoi(tokens[1]);
            std::string var_name = tokens[2];
            std::string data_type = tokens[3];
            int num_elements = std::stoi(tokens[4]);
            DataType type = DataType::Char;
            bool valid_type = true;
            if(data_type == "char")
                type = DataType::Char;
            else if(data_type == "short")
                type = DataType::Short;
            else if(data_type == "int")
                type = DataType::Int;
            else if(data_type == "float")
                type = DataType::Float;
            else if(data_type == "long")
                type = DataType::Long;
            else if(data_type == "double")
                type = DataType::Double;
            else
                valid_type = false;

            if (valid_type)
                allocateVariable(pid, var_name, type, num_elements, mmu, page_table);
            else
                std::cout << "error: command not recognized" << std::endl;
        }
        else if(cmd == "set" && tokens.size() >= 5){
            int pid = std::stoi(tokens[1]);
            std::string var_name = tokens[2];
            int offset = std::stoi(tokens[3]);

            if (!mmu->pidExists(pid)){
                printf("error: process not found\n");
            }
            else if (!mmu->variableExists(pid, var_name)){
                printf("error: variable not found\n");
            }
            else {
                Variable *var = mmu->getVariable(pid, var_name);
                uint32_t type_size = getTypeSize(var->type);
                uint32_t num_values = tokens.size() - 4;
                uint32_t capacity = (type_size == 0) ? 0 : var->size / type_size;

                if (offset + num_values > capacity){
                    printf("error: index out of range\n");
                }
                else{
                    for (size_t i = 0; i < num_values; i++){
                        setVariable(pid, var_name, offset + i, tokens[4 + i], mmu, page_table, memory);
                    }
                }
            }
        }
        else if(cmd == "free" && tokens.size() >= 3){
            int pid = std::stoi(tokens[1]);
            std::string var_name = tokens[2];
            freeVariable(pid, var_name, mmu, page_table);
        }
        else if(cmd == "terminate" && tokens.size() >= 2){
            int pid = std::stoi(tokens[1]);
            terminateProcess(pid, mmu, page_table);
        }
        else if(cmd == "print" && tokens.size() >= 2){
            std::string object = tokens[1];
            /*  * print <object> (prints data)
                * If <object> is "mmu", print the MMU memory table
                * if <object> is "page", print the page table
                * if <object> is "processes", print a list of PIDs for processes that are still running
                * if <object> is a "<PID>:<var_name>", print the value of the variable for that process*/

            if(object == "mmu")
                mmu->print();
            else if (object == "page"){
                page_table->print();
            }
            else if (object == "processes"){
                printProcesses(mmu);
            }
            else if (object.find(':') != std::string::npos) {
                int pid = std::stoi(object.substr(0,object.find(':')));
                std::string var_name = object.substr(object.find(':') + 1);

                printVariableValue(pid, var_name, mmu, page_table, memory);
            }
            else{
                std::cout << "error: command not recognized" << std::endl;
            }
        }
        else{
            std::cout << "error: command not recognized" << std::endl;
        }
    }

    // Cean up
    delete[] memory;
    delete mmu;
    delete page_table;

    return 0;
}

bool isValidPageSize(int page_size)
{
    if (page_size < 1024 || page_size > 32768)
    {
        return false;
    }

    return (page_size & (page_size - 1)) == 0;
}

std::vector<std::string> splitCommand(const std::string &command)
{
    std::vector<std::string> tokens;
    std::stringstream buffer(command);
    std::string token;

    while (buffer >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

uint32_t getTypeSize(DataType type)
{
    if(type == DataType::Char)
        return 1;
    if(type == DataType::Short)
        return 2;
    if(type == DataType::Int || type == DataType::Float)
        return 4;
    if(type == DataType::Long || type == DataType::Double)
        return 8;
    return 0;
}

std::vector<uint8_t> readBytes(uint32_t pid, uint32_t virtual_address, uint32_t size, PageTable *page_table, uint8_t *memory)
{
    std::vector<uint8_t> bytes(size);

    for (uint32_t i = 0; i < size; i++)
    {
        int physical_address = page_table->getPhysicalAddress(pid, virtual_address + i);
        bytes[i] = memory[physical_address];
    }

    return bytes;
}

void printStartMessage(int page_size)
{
    std::cout << "Welcome to the Memory Allocation Simulator! Using a page size of " << page_size << " bytes." << std:: endl;
    std::cout << "Commands:" << std:: endl;
    std::cout << "  * create <text_size> <data_size> (initializes a new process)" << std:: endl;
    std::cout << "  * allocate <PID> <var_name> <data_type> <number_of_elements> (allocated memory on the heap)" << std:: endl;
    std::cout << "  * set <PID> <var_name> <offset> <value_0> <value_1> <value_2> ... <value_N> (set the value for a variable)" << std:: endl;
    std::cout << "  * free <PID> <var_name> (deallocate memory on the heap that is associated with <var_name>)" << std:: endl;
    std::cout << "  * terminate <PID> (kill the specified process)" << std:: endl;
    std::cout << "  * print <object> (prints data)" << std:: endl;
    std::cout << "    * If <object> is \"mmu\", print the MMU memory table" << std:: endl;
    std::cout << "    * if <object> is \"page\", print the page table" << std:: endl;
    std::cout << "    * if <object> is \"processes\", print a list of PIDs for processes that are still running" << std:: endl;
    std::cout << "    * if <object> is a \"<PID>:<var_name>\", print the value of the variable for that process" << std:: endl;
    std::cout << std::endl;
}

void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table)
{
    //   - create new process in the MMU
    //   - allocate new variables for the <TEXT>, <GLOBALS>, and <STACK>
    //   - print pid
    uint32_t startup_size = text_size + data_size + 65536;
    uint32_t page_size = page_table->getPageSize();
    uint32_t required_pages = (startup_size + page_size - 1) / page_size;
    uint32_t max_frames = PHYSICAL_MEMORY / page_size;

    if (page_table->getEntryCount() + required_pages > max_frames)
    {
        printf("error: not enough memory\n");
        return;
    }

    int pid = mmu->createProcess();
    allocateVariable(pid, "<TEXT>", DataType::Char, text_size, mmu, page_table);
    allocateVariable(pid, "<GLOBALS>", DataType::Char, data_size, mmu, page_table);
    allocateVariable(pid, "<STACK>", DataType::Char, 65536, mmu, page_table);
    printf("%d\n", pid);
}

void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table)
{
    //   - find first free space within a page already allocated to this process that is large enough to fit the new variable
    //   - if no hole is large enough, allocate new page(s)
    //   - insert variable into MMU
    //   - print virtual memory address

    // check if the PID does not exist
    if(!mmu->pidExists(pid)){
        printf("error: process not found\n");
        return;
    }
    if (mmu->variableExists(pid, var_name)) {
        printf("error: variable already exists\n");
        return;
    }

    uint32_t size = getTypeSize(type) * num_elements;

    // find next virtual address
    uint32_t address = mmu->getAvailableAddress(pid, size);
    if (address + size > PHYSICAL_MEMORY) {
        printf("error: not enough memory\n");
        return;
    }

    // allocate new pages if no space
    uint32_t page_size = page_table->getPageSize();
    std::vector<int> pages_to_add;

    if (size > 0)
    {
        uint32_t start_page = address / page_size;
        uint32_t end_page = (address + size - 1) / page_size;

        for(uint32_t page = start_page; page <= end_page; page++){
            if (!page_table->hasEntry(pid, page)){
                pages_to_add.push_back(page);
            }
        }
    }

    uint32_t max_frames = PHYSICAL_MEMORY / page_size;
    if (page_table->getEntryCount() + pages_to_add.size() > max_frames) {
        printf("error: not enough memory\n");
        return;
    }

    for(size_t i = 0; i < pages_to_add.size(); i++){
        page_table->addEntry(pid, pages_to_add[i]);
    }

    mmu->addVariableToProcess(pid, var_name, type, size, address);

    // print address of newly created variable except for <TEXT> and so on
    if(var_name != "<TEXT>" && var_name != "<GLOBALS>" && var_name != "<STACK>")
        printf("%d\n", address);
    
}

void setVariable(uint32_t pid, std::string var_name, uint32_t offset, std::string value, Mmu *mmu, PageTable *page_table, uint8_t *memory)
{
    //   - look up physical address for variable based on its virtual address / offset
    //   - insert `value` into `memory` at physical address
    //   * note: this function only handles a single element (i.e. you'll need to call this within a loop when setting
    //           multiple elements of an array)
    if (!mmu->pidExists(pid)){
        printf("error: process not found\n");
        return;
    }
    if (!mmu->variableExists(pid, var_name)){
        printf("error: variable not found\n");
        return;
    }

    Variable *var = mmu->getVariable(pid, var_name);
    uint32_t type_size = getTypeSize(var->type);
    uint32_t byte_offset = offset * type_size;

    if (byte_offset + type_size > var->size){
        printf("error: index out of range\n");
        return;
    }

    uint32_t virtual_address = var->virtual_address + byte_offset;
    uint8_t bytes[8] = {0};

    if(var->type == DataType::Char){
        char parsed = value[0];
        std::memcpy(bytes, &parsed, type_size);
    }
    else if(var->type == DataType::Short){
        int16_t parsed = static_cast<int16_t>(std::stoi(value));
        std::memcpy(bytes, &parsed, type_size);
    }
    else if(var->type == DataType::Int){
        int32_t parsed = static_cast<int32_t>(std::stoi(value));
        std::memcpy(bytes, &parsed, type_size);
    }
    else if(var->type == DataType::Float){
        float parsed = std::stof(value);
        std::memcpy(bytes, &parsed, type_size);
    }
    else if(var->type == DataType::Long){
        int64_t parsed = std::stoll(value);
        std::memcpy(bytes, &parsed, type_size);
    }
    else if(var->type == DataType::Double){
        double parsed = std::stod(value);
        std::memcpy(bytes, &parsed, type_size);
    }

    for (uint32_t i = 0; i < type_size; i++)
    {
        int physical_address = page_table->getPhysicalAddress(pid, virtual_address + i);
        memory[physical_address] = bytes[i];
    }
}

void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table)
{
    //   - remove entry from MMU
    //   - free page if this variable was the only one on a given page
    if (!mmu->pidExists(pid)){
        printf("error: process not found\n");
        return;
    }
    if (!mmu->variableExists(pid, var_name)){
        printf("error: variable not found\n");
        return;
    }

    Variable *var = mmu->getVariable(pid, var_name);
    uint32_t page_size = page_table->getPageSize();
    std::vector<uint32_t> pages;

    if (var->size > 0)
    {
        uint32_t start_page = var->virtual_address / page_size;
        uint32_t end_page = (var->virtual_address + var->size - 1) / page_size;

        for (uint32_t page = start_page; page <= end_page; page++)
        {
            pages.push_back(page);
        }
    }

    mmu->removeVariableFromProcess(pid, var_name);

    for (size_t i = 0; i < pages.size(); i++)
    {
        if (!mmu->pageHasAllocatedVariable(pid, pages[i], page_size))
        {
            page_table->removeEntry(pid, pages[i]);
        }
    }
}

void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table)
{
    //   - remove process from MMU
    //   - free all pages associated with given process
    if (!mmu->pidExists(pid)){
        printf("error: process not found\n");
        return;
    }

    page_table->removeEntries(pid);
    mmu->removeProcess(pid);
}

void printProcesses(Mmu *mmu)
{
    std::vector<uint32_t> pids = mmu->getProcessIds();

    for (size_t i = 0; i < pids.size(); i++)
    {
        printf("%d\n", pids[i]);
    }
}

void printVariableValue(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table, uint8_t *memory)
{
    if (!mmu->pidExists(pid)){
        printf("error: process not found\n");
        return;
    }
    if (!mmu->variableExists(pid, var_name)){
        printf("error: variable not found\n");
        return;
    }

    Variable *var = mmu->getVariable(pid, var_name);
    uint32_t type_size = getTypeSize(var->type);
    uint32_t count = (type_size == 0) ? 0 : var->size / type_size;
    uint32_t limit = (count > 4) ? 4 : count;

    for (uint32_t i = 0; i < limit; i++)
    {
        std::vector<uint8_t> bytes = readBytes(pid, var->virtual_address + i * type_size, type_size, page_table, memory);

        if(var->type == DataType::Char){
            char value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }
        else if(var->type == DataType::Short){
            int16_t value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }
        else if(var->type == DataType::Int){
            int32_t value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }
        else if(var->type == DataType::Float){
            float value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }
        else if(var->type == DataType::Long){
            int64_t value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }
        else if(var->type == DataType::Double){
            double value;
            std::memcpy(&value, &bytes[0], type_size);
            std::cout << value;
        }

        if (i + 1 < limit)
        {
            std::cout << ", ";
        }
    }

    if (count > 4)
    {
        if (limit > 0)
        {
            std::cout << ", ";
        }
        std::cout << "... [" << count << " items]";
    }

    std::cout << std::endl;
}

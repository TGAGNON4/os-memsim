#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include "mmu.h"
#include "pagetable.h"

// 64 MB (64 * 1024 * 1024)
#define PHYSICAL_MEMORY 67108864

void printStartMessage(int page_size);
void createProcess(int text_size, int data_size, Mmu *mmu, PageTable *page_table);
void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table);
void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory);
void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table);
void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table);

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
    printStartMessage(page_size);

    // Create physical 'memory' (raw array of bytes)
    uint8_t *memory = new uint8_t[PHYSICAL_MEMORY];

    // Create MMU and Page Table
    Mmu *mmu = new Mmu(PHYSICAL_MEMORY);
    PageTable *page_table = new PageTable(page_size);

    // Prompt loop
    std::string command;
    std::cout << "> ";
    std::getline(std::cin, command);
    while (command != "exit")
    {
        std::string cmd = strtok(&command[0], " ");
        // Handle command
        // TODO: implement this!
        if(cmd == "create"){
            int text_size = std::stoi(strtok(NULL, " ")); // gets next argument
            int data_size = std::stoi(strtok(NULL, " ")); // gets final argument
            createProcess(text_size, data_size, mmu, page_table);
        }
        else if(cmd == "allocate"){
            int pid = std::stoi(strtok(NULL, " "));
            std::string var_name = strtok(NULL, " ");
            std::string data_type = strtok(NULL, " ");
            int num_elements = std::stoi(strtok(NULL, " "));
            DataType type;
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
            allocateVariable(pid, var_name, type, num_elements, mmu, page_table);
        }
        else if(cmd == "set"){
            int pid = std::stoi(strtok(NULL, " "));
            std::string var_name = strtok(NULL, " ");
            int offset = std::stoi(strtok(NULL, " "));
            int *values = new int[1];
            int i = 0;
            while(true){
                if(strtok(NULL, " ") == NULL)
                    break;
                else{
                    values[i] = std::stoi(strtok(NULL, " "));
                    i++;
                }
            }
            setVariable(pid, var_name, offset, (void*)values, mmu, page_table, memory);
            delete[] values;
        }
        else if(cmd == "free"){
            int pid = std::stoi(strtok(NULL, " "));
            std::string var_name = strtok(NULL, " ");
            freeVariable(pid, var_name, mmu, page_table);
        }
        else if(cmd == "terminate"){
            int pid = std::stoi(strtok(NULL, " "));
            terminateProcess(pid, mmu, page_table);
        }
        else if(cmd == "print"){
            std::string object = strtok(NULL, " ");

            // some print statement logic were "object" can be "mmu", "page", "processes", or "<PID>:<var_name>"
        }
        else{
            std::cout << "error: command not recognized" << std::endl;
        }

        // Get next command
        std::cout << "> ";
        std::getline(std::cin, command);
    }

    // Cean up
    delete[] memory;
    delete mmu;
    delete page_table;

    return 0;
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
    // TODO: implement this!
    //   - create new process in the MMU
    //   - allocate new variables for the <TEXT>, <GLOBALS>, and <STACK>
    //   - print pid
}

void allocateVariable(uint32_t pid, std::string var_name, DataType type, uint32_t num_elements, Mmu *mmu, PageTable *page_table)
{
    // TODO: implement this!
    //   - find first free space within a page already allocated to this process that is large enough to fit the new variable
    //   - if no hole is large enough, allocate new page(s)
    //   - insert variable into MMU
    //   - print virtual memory address
}

void setVariable(uint32_t pid, std::string var_name, uint32_t offset, void *value, Mmu *mmu, PageTable *page_table, uint8_t *memory)
{
    // TODO: implement this!
    //   - look up physical address for variable based on its virtual address / offset
    //   - insert `value` into `memory` at physical address
    //   * note: this function only handles a single element (i.e. you'll need to call this within a loop when setting
    //           multiple elements of an array)
}

void freeVariable(uint32_t pid, std::string var_name, Mmu *mmu, PageTable *page_table)
{
    // TODO: implement this!
    //   - remove entry from MMU
    //   - free page if this variable was the only one on a given page
}

void terminateProcess(uint32_t pid, Mmu *mmu, PageTable *page_table)
{
    // TODO: implement this!
    //   - remove process from MMU
    //   - free all pages associated with given process
}

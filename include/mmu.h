#ifndef __MMU_H_
#define __MMU_H_

#include <cstdint>
#include <string>
#include <vector>

enum DataType : uint8_t {FreeSpace, Char, Short, Int, Float, Long, Double};

typedef struct Variable {
    std::string name;
    DataType type;
    uint32_t virtual_address;
    uint32_t size;
} Variable;

typedef struct Process {
    uint32_t pid;
    std::vector<Variable*> variables;
} Process;

class Mmu {
private:
    uint32_t _next_pid;
    uint32_t _max_size;
    std::vector<Process*> _processes;

    Process* findProcess(uint32_t pid);
    void sortVariables(Process *proc);
    void coalesceFreeSpaces(Process *proc);

public:
    Mmu(int memory_size);
    ~Mmu();

    uint32_t createProcess();
    void addVariableToProcess(uint32_t pid, std::string var_name, DataType type, uint32_t size, uint32_t address);
    Variable* getVariable(uint32_t pid, std::string var_name);
    std::vector<Variable*> getVariables(uint32_t pid);
    std::vector<uint32_t> getProcessIds();
    void removeVariableFromProcess(uint32_t pid, std::string var_name);
    void removeProcess(uint32_t pid);
    bool pageHasAllocatedVariable(uint32_t pid, uint32_t page_number, uint32_t page_size);
    void print();
    uint32_t getAvailableAddress(uint32_t pid, uint32_t size);
    bool pidExists(uint32_t pid);
    bool variableExists(uint32_t pid, std::string var_name); // always call this after pidExists()
};

#endif // __MMU_H_

#include <iostream>
#include <algorithm>
#include "mmu.h"

Mmu::Mmu(int memory_size)
{
    _next_pid = 1024;
    _max_size = memory_size;
}

Mmu::~Mmu()
{
}

uint32_t Mmu::createProcess()
{
    Process *proc = new Process();
    proc->pid = _next_pid;

    Variable *var = new Variable();
    var->name = "<FREE_SPACE>";
    var->type = DataType::FreeSpace;
    var->virtual_address = 0;
    var->size = _max_size;
    proc->variables.push_back(var);

    _processes.push_back(proc);

    _next_pid++;
    
    return proc->pid;
}

void Mmu::addVariableToProcess(uint32_t pid, std::string var_name, DataType type, uint32_t size, uint32_t address)
{
    std::vector<Process*>::iterator it = std::find_if(_processes.begin(), _processes.end(), [pid](Process* p)
    { 
        return p != nullptr && p->pid == pid; 
    });
    
    if (it != _processes.end())
    {
        Process *proc = *it;
        Variable *var = new Variable();
        var->name = var_name;
        var->type = type;
        var->virtual_address = address;
        var->size = size;
        proc->variables.push_back(var);
    }
}

void Mmu::print()
{
    int i, j;

    std::cout << " PID  | Variable Name | Virtual Addr | Size" << std::endl;
    std::cout << "------+---------------+--------------+------------" << std::endl;
    for (i = 0; i < _processes.size(); i++)
    {
        for (j = 0; j < _processes[i]->variables.size(); j++)
        {
            if(_processes[i]->variables[j]->type == DataType::FreeSpace)
                continue;
            
            printf(" %4d | %-13s |   0x%08X | %10d\n",
                _processes[i]->pid,
                _processes[i]->variables[j]->name.c_str(),
                _processes[i]->variables[j]->virtual_address,
                _processes[i]->variables[j]->size);
        }
    }
}


uint32_t Mmu::getAvailableAddress(uint32_t pid){
    std::vector<Process*>::iterator it = std::find_if(_processes.begin(), _processes.end(), [pid](Process* p)
    { 
        return p != nullptr && p->pid == pid; 
    });

    // find what the next available address it
    uint32_t address = 0;
    if (it != _processes.end()) {
        std::vector<Variable*>::iterator vit;
        for (vit = (*it)->variables.begin(); vit != (*it)->variables.end(); vit++) {
            if((*vit)->type == DataType::FreeSpace)
                continue; // skip free spaces
            uint32_t end = (*vit)->virtual_address + (*vit)->size;
            if (end > address) 
                address = end;
        }
    }
    return address;
}

// have to implement here as it has access to _processes
bool Mmu::pidExists(uint32_t pid){
    std::vector<Process*>::iterator it = std::find_if(_processes.begin(), _processes.end(), [pid](Process* p)
    { 
        return p != nullptr && p->pid == pid; 
    });
    return it != _processes.end(); // if iterator is at the end then PID does not exist
}

// always call this after pidExists()
bool Mmu::variableExists(uint32_t pid, std::string var_name){
    std::vector<Process*>::iterator it = std::find_if(_processes.begin(), _processes.end(), [pid](Process* p)
    { 
        return p != nullptr && p->pid == pid; 
    });
    
    // assuming iterator found pid since pidExists() will be called first
    std::vector<Variable*>::iterator vit;
    for (vit = (*it)->variables.begin(); vit != (*it)->variables.end(); vit++) {
        if((*vit)->name == var_name)
            return true;
    }
    return false;
}

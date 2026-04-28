#include <iostream>
#include <algorithm>
#include "mmu.h"

Process* Mmu::findProcess(uint32_t pid)
{
    std::vector<Process*>::iterator it = std::find_if(_processes.begin(), _processes.end(), [pid](Process *p)
    {
        return p != nullptr && p->pid == pid;
    });

    if (it == _processes.end())
    {
        return nullptr;
    }

    return *it;
}

void Mmu::sortVariables(Process *proc)
{
    std::sort(proc->variables.begin(), proc->variables.end(), [](Variable *a, Variable *b)
    {
        if (a->virtual_address != b->virtual_address)
        {
            return a->virtual_address < b->virtual_address;
        }
        return a->type < b->type;
    });
}

void Mmu::coalesceFreeSpaces(Process *proc)
{
    sortVariables(proc);

    for (size_t i = 0; i + 1 < proc->variables.size();)
    {
        Variable *current = proc->variables[i];
        Variable *next = proc->variables[i + 1];

        if (current->type == DataType::FreeSpace &&
            next->type == DataType::FreeSpace &&
            current->virtual_address + current->size == next->virtual_address)
        {
            current->size += next->size;
            delete next;
            proc->variables.erase(proc->variables.begin() + i + 1);
        }
        else
        {
            i++;
        }
    }
}

Mmu::Mmu(int memory_size)
{
    _next_pid = 1024;
    _max_size = memory_size;
}

Mmu::~Mmu()
{
    for (size_t i = 0; i < _processes.size(); i++)
    {
        for (size_t j = 0; j < _processes[i]->variables.size(); j++)
        {
            delete _processes[i]->variables[j];
        }
        delete _processes[i];
    }
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
    Process *proc = findProcess(pid);

    if (proc == nullptr)
    {
        return;
    }

    for (size_t i = 0; i < proc->variables.size(); i++)
    {
        Variable *free_var = proc->variables[i];
        uint32_t free_start = free_var->virtual_address;
        uint32_t free_end = free_start + free_var->size;
        uint32_t alloc_end = address + size;

        if (free_var->type != DataType::FreeSpace)
        {
            continue;
        }

        if (address < free_start || alloc_end > free_end)
        {
            continue;
        }

        Variable *var = new Variable();
        var->name = var_name;
        var->type = type;
        var->virtual_address = address;
        var->size = size;

        if (address == free_start && alloc_end == free_end)
        {
            delete free_var;
            proc->variables[i] = var;
        }
        else if (address == free_start)
        {
            free_var->virtual_address = alloc_end;
            free_var->size = free_end - alloc_end;
            proc->variables.push_back(var);
        }
        else if (alloc_end == free_end)
        {
            free_var->size = address - free_start;
            proc->variables.push_back(var);
        }
        else
        {
            Variable *tail = new Variable();
            tail->name = "<FREE_SPACE>";
            tail->type = DataType::FreeSpace;
            tail->virtual_address = alloc_end;
            tail->size = free_end - alloc_end;

            free_var->size = address - free_start;
            proc->variables.push_back(var);
            proc->variables.push_back(tail);
        }

        sortVariables(proc);
        return;
    }
}

Variable* Mmu::getVariable(uint32_t pid, std::string var_name)
{
    Process *proc = findProcess(pid);

    if (proc == nullptr)
    {
        return nullptr;
    }

    for (size_t i = 0; i < proc->variables.size(); i++)
    {
        if (proc->variables[i]->name == var_name)
        {
            return proc->variables[i];
        }
    }

    return nullptr;
}

std::vector<Variable*> Mmu::getVariables(uint32_t pid)
{
    Process *proc = findProcess(pid);

    if (proc == nullptr)
    {
        return std::vector<Variable*>();
    }

    sortVariables(proc);
    return proc->variables;
}

std::vector<uint32_t> Mmu::getProcessIds()
{
    std::vector<uint32_t> pids;

    for (size_t i = 0; i < _processes.size(); i++)
    {
        pids.push_back(_processes[i]->pid);
    }

    return pids;
}

void Mmu::removeVariableFromProcess(uint32_t pid, std::string var_name)
{
    Variable *var = getVariable(pid, var_name);

    if (var == nullptr)
    {
        return;
    }

    var->name = "<FREE_SPACE>";
    var->type = DataType::FreeSpace;

    Process *proc = findProcess(pid);
    if (proc != nullptr)
    {
        coalesceFreeSpaces(proc);
    }
}

void Mmu::removeProcess(uint32_t pid)
{
    for (std::vector<Process*>::iterator it = _processes.begin(); it != _processes.end(); it++)
    {
        if ((*it)->pid == pid)
        {
            for (size_t i = 0; i < (*it)->variables.size(); i++)
            {
                delete (*it)->variables[i];
            }
            delete *it;
            _processes.erase(it);
            return;
        }
    }
}

bool Mmu::pageHasAllocatedVariable(uint32_t pid, uint32_t page_number, uint32_t page_size)
{
    Process *proc = findProcess(pid);

    if (proc == nullptr)
    {
        return false;
    }

    uint32_t page_start = page_number * page_size;
    uint32_t page_end = page_start + page_size;

    for (size_t i = 0; i < proc->variables.size(); i++)
    {
        Variable *var = proc->variables[i];
        uint32_t var_start = var->virtual_address;
        uint32_t var_end = var_start + var->size;

        if (var->type == DataType::FreeSpace)
        {
            continue;
        }

        if (var_start < page_end && page_start < var_end)
        {
            return true;
        }
    }

    return false;
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


uint32_t Mmu::getAvailableAddress(uint32_t pid, uint32_t size)
{
    Process *proc = findProcess(pid);

    if (proc == nullptr)
    {
        return _max_size;
    }

    sortVariables(proc);

    for (size_t i = 0; i < proc->variables.size(); i++)
    {
        if (proc->variables[i]->type == DataType::FreeSpace &&
            proc->variables[i]->size >= size)
        {
            return proc->variables[i]->virtual_address;
        }
    }

    return _max_size;
}

// have to implement here as it has access to _processes
bool Mmu::pidExists(uint32_t pid){
    return findProcess(pid) != nullptr;
}

// always call this after pidExists()
bool Mmu::variableExists(uint32_t pid, std::string var_name){
    return getVariable(pid, var_name) != nullptr;
}

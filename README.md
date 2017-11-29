##SIMU NUMA ON ONE NODE MACHINE

###SETUP
* put this project under ~

###TODO
* update main.cpp to simulate NUMA environment
* generate llvm bc and find the corresponding instr with respect to the user code

Assumption:
1. two threads

Find the pair of thread and data(address):
1. from thread_create, get thread name
2. then, we know the global data(address) mentioned in the function are binded to the thread (but we may don't know tid/cpu_id)
3. use data structure to record the mapping
4. find intersection of all mapping (use heuristic)

If they has intersection, set to the same code

Future work
1. sequence execution
2. random execution

* runOnFunction?

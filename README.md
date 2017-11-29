## Project: NAME TBD

### Background
On NUMA, we find the problem...
We simulate the problem on non-NUMA machine by...
We write some testcases...
After running our LLVM pass(es), the performance improved by...

### Related Works

### Setup
* put this project under ~

### Assumptions
1. our testcases starts with two threads
1. threads are executed in sequence order

### Algorithm
Find the pair of thread and data(address):
1. get thread name and function name from thread_create, and create mapping btw them
1. find the global data(address) mentioned in thread function
1. use data structure to record the mapping (maybe function_name -> global_addresses)
1. find intersections of all mappings
1. If they has any intersection, set those threads to the same core

### Future works
1. sequence execution
1. random execution
 * use heuristic to find the intersection
1. complex global data share pattern

---

## Log

### Problems
* Which LLVM pass should be used? How many passes should be used?
* How to identify global data?

### Solved Problems

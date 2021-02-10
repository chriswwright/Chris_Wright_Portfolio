#A compilation of work from CPE453 that I deemed technically impressive.

##Program 2: Light-weight process API
*There are three demo functions named: numbers, snakes, and hungry
*Utilizes context switching to swap between processes.
*Operable with various scheduling algorithms
*Limitation with LWP is that the processes must yield to the next thread
**This can lead to deadlocks

##Program 3: Memory Simulation
*Simulation of a memory management units operation
*Contains a backing storage, TLB, page table, address list and MMU
*The simulation takes four arguments
**A file containing a list of addressess
**Number of frames in main memory
**Number of pages in page table
**An page replacement algorithm IE: FIFO, LRU, OPT, SEC

##Program 4: TinyFS File System
*A file system API that operates on a disk simulator.
*The disk simulator API contains the following function calls:
**openDisk, closeDisk, readBlock, writeBlock

Additional information and justification for my project4 design decisions can 
be found within the project4 directory ReadMe file.

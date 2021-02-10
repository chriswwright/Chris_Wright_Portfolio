from page_table import *
from tlb import tlb
from memory import memory
from sys import argv

'''
Simulates a memory with TLB and paging, reading from a binary
input file
inputs:
  ref_sequence: Filename that contains the list of memory addresses
                that will be used for the memory access.
  frames: the number of frames in physical memory.
  pra: page replacement algorithm. 0 = FIFO, 1 = LRU, 2 = OPT, 3 = Second Chance
'''
def memSim(ref_sequence, frames = 256, pra = 0):
    #split values from the file ref_sequence
    page_list = [];
    offset_list = [];

    with open(ref_sequence , "r") as seq:
        for line in seq:
            address =int(line)
            page_list.append(address//256)
            offset_list.append(address%256)
    


    #Memory structure creation
    tlb_l = tlb()
    page_table_l = page_table(pra)
    sec_mem = open("BACKING_STORE.bin", "rb")
    phys_mem = memory(sec_mem, frames)
    num_addresses = len(page_list)
    num_tlb_hits = 0
    num_tlb_misses = 0
    num_page_fault = 0

    #memory unit main loop
    while len(page_list) != 0:
        tlb_miss = False
        page_fault = False
        page_num = page_list[0]
        offset = offset_list[0]
        '''
        page_backup = page_list.copy()
        offset_backup = offset_list.copy()
        '''
        page_list.pop(0)
        offset_list.pop(0)
        
        #check for tlb hit
        frame_number = tlb_l.get(page_num)
        if frame_number == -1:
            num_tlb_misses += 1

        #check for page_table hit
            frame_number = page_table_l.get_page(page_num)
        else:
            num_tlb_hits += 1
            if pra == 1:
                page_table_l.lru_swap(page_num)
            elif pra == 3:
                page_table_l.sec_set(page_num)

        if frame_number == -1:

            #page fault
            num_page_fault += 1
            if phys_mem.size == phys_mem.max_size:
                #page replacement
                if pra == 2:
                    target_info = page_table_l.void_page(page_list)
                else:
                    target_info = page_table_l.void_page()
                frame_number = target_info[1]
                tlb_l.void(target_info[0])
                phys_mem.swap_data(page_num, frame_number)
                page_table_l.load_page(page_num, frame_number)
                tlb_l.load(page_num, frame_number)
                
            else:
                #page load
                frame_number = phys_mem.append_data(page_num)
                page_table_l.load_page(page_num, frame_number)
                tlb_l.load(page_num, frame_number)


        #write out to file
        data = phys_mem.get_data(frame_number)
        value = data[offset]
        if value > 127:
            value -= 256
        address = str(256*page_num+offset)
        d_string = ""
        for byte in data:
            d_string += "{:0>2X}".format(byte)
        p_string = "{}, {:-}, {}, {}".format(address, value, frame_number, d_string)
        print(p_string)
        '''
        page_list = page_backup
        offset_list = offset_backup
        '''

    print("Number of Translated Addresses = {}".format(num_addresses))
    print("Page Faults = {}".format(num_page_fault))
    print("Page Fault Rate = {:.3f}".format(num_page_fault / num_addresses))
    print("TLB Hits = {}".format(num_tlb_hits))
    print("TLB Misses = {}".format(num_tlb_misses))
    print("TLB Hit Rate = {:.3f}".format(num_tlb_hits / num_addresses))
    return 0

if __name__ == '__main__':
    if (len(argv) >= 2) and (len(argv) <=4):
        filename = argv[1]
        frame_no = 256
        pra = 0
        if len(argv) > 2:
            frame_no = int(argv[2])
        if len(argv) > 3:
            if argv[3] == "fifo":
                pra = 0
            elif argv[3] == "lru":
                pra = 1
            elif argv[3] == "opt":
                pra = 2
            elif argv[3] == "sec":
                pra = 3
        memSim(filename, frame_no, pra)
    else:
        print("usage: memSim <reference-sequence-file.txt> <FRAMES> <PRA>")



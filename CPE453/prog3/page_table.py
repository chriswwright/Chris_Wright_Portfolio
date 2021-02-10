class page_entry:
    def __init__(self, page_no, frame_no):
        self.page_number = page_no;
        self.frame_number = frame_no;
        self.loaded = False;
        self.ref_bit = False;

class page_table:
    def __init__(self, rep_alg):
        self.rep_queue = []
        self.data = []
        self.max_size = 256
        self.rep_alg = rep_alg
        for i in range(0, self.max_size):
            entry = page_entry(i, -1)
            self.data.append(entry)

    '''
    returns frame number of page located at the index.
    returns -1 if the page is not currently loaded as a frame.
    '''
    def get_page(self, page_no):
        if self.data[page_no].loaded == True:
            if self.rep_alg == 1:
                self.lru_swap(page_no)
            elif self.rep_alg == 3:
                self.sec_set(page_no)
            return self.data[page_no].frame_number
        return -1
    

    '''
    updates the frame number of a page specified by page_no with provided frame_no
    returns 0 on success and -1 on failure
    '''
    def load_page(self, page_no, frame_no):
        if self.data[page_no].loaded == True:
            return -1
        self.data[page_no].loaded = True
        self.data[page_no].frame_number = frame_no
        self.rep_queue.append(self.data[page_no])
        return 0

    '''
    pops page from replacement queue and 
    clears the loaded states of popped page 
    returns page_number on success and -1 on failure
    '''
    def void_page(self, remaining = 0):
        if(self.rep_alg == 2):
            self.reorder_optimal(remaining)
        elif(self.rep_alg == 3):
            self.reorder_sec()
        if self.rep_queue[0].loaded == False:
            return -1
        victim = self.rep_queue[0]
        ret_frame = self.data[victim.page_number].frame_number
        self.data[victim.page_number].loaded = False
        self.data[victim.page_number].frame_number = -1
        self.rep_queue.pop(0)
        return [victim.page_number, ret_frame]


    '''
    reorders rep_queue following optimal replacement algorithm following a list
    of remaining page accesses
    '''
    def reorder_optimal(self, remaining):
        #note lists are passed by refrence needing a copy op if the list needs to be preserved    
        current_pages = self.rep_queue.copy()
        max = 0
        target = 0
        i = 0
        for i in range(len(current_pages)):
            current = 0
            for rem_page in remaining:
                if current_pages[i].page_number == rem_page:
                    break
                current += 1
            if current == len(remaining):
                swap = self.rep_queue[0]
                self.rep_queue[0] = self.rep_queue[i]
                self.rep_queue[i] = swap
                return 0
            elif current > max:
                max = current
                target = i
        swap = self.rep_queue[0]
        self.rep_queue[0] = self.rep_queue[target]
        self.rep_queue[target] = swap
        return 0

    def reorder_sec(self):
        found = False
        while not found:
            for i in range(len(self.rep_queue)):
                target = self.rep_queue[i]
                if target.ref_bit == False:
                    found = True
                    break
                else:
                    target.ref_bit = False
        swap = self.rep_queue.pop(i)
        self.rep_queue.insert(0, swap)
        

    def sec_set(self, page_no):
        for i in range(len(self.rep_queue)):
            if self.rep_queue[i].page_number == page_no:
                self.rep_queue[i].ref_bit = True

    def lru_swap(self, page_no):
        for i in range(len(self.rep_queue)):
            if self.rep_queue[i].page_number == page_no:
                self.rep_queue.pop(i)
                self.rep_queue.append(self.data[page_no])

from page_table import page_entry

class tlb:
    def __init__(self):
        self.data = []   #all pages in the TLB
        self.size = 16
        
    '''
    returns -1 if the page is not in the TLB,
    returns the frame number on TLB hit.
    '''
    def get(self, page):	
        for entry in self.data:
            if page == entry.page_number and entry.loaded == True:            
                return entry.frame_number

        return -1

    '''
    appends a new page-frame translation to self.data.
    if more than 16 translations are in the tlb remove the first
    entry (following FIFO replacement strategy)
    retuns 0 on success, -1 on failure.
    '''
    def load(self, page, frame):
        new_entry = page_entry(page, frame)
        new_entry.loaded = True
        for entry in self.data:
            if entry.page_number == page:
                return -1
        if len(self.data) == self.size:
            self.data.pop(0)
        self.data.append(new_entry)
        return 0

    '''
    voids given page. Returns 0 on successful void or -1 on failue
    '''
    def void(self, page):
        for i in range(len(self.data)):
            if page == self.data[i].page_number:
                if self.data[i].loaded == False:
                    return -1
                self.data[i].page_number = -1
                self.data[i].frame_number = -1
                self.data[i].loaded = False
                return 0
        return -1


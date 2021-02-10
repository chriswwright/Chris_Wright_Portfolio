class memory:
    def __init__(self, source, num_frames = 256):
        self.data = []
        self.max_size = num_frames
        self.size = 0
        self.source = source   #Source is a binary file object to be read from by the memory class

    '''
    returns the 256 byte value string stored at location frame_no
    returns -1 if frame_no is more than size, or if nothing is stored there
    '''
    def get_data(self, frame_no):
        if (frame_no >= self.max_size) or (frame_no >= self.size):
            return -1
        return self.data[frame_no]

    '''
    loads 256 bytes from location page_no*256 from secondary memory and loads
    it at self.data[frame_no]
    returns -1 if frame_no is an invalid address
    '''
    def swap_data(self, page_no, frame_no):
        if (frame_no >= self.max_size) or (frame_no >= self.size):
            return -1
        self.source.seek(page_no * 256)
        self.data[frame_no] = self.source.read(256)
        return 0

    '''
    loads 256 bytes from the location page_no*256 from secondary memory and
    appends that data to the end of self.data. Returns the index where the data
    was placed
    '''
    def append_data(self, page_no):
        self.source.seek(page_no * 256)
        self.data.append(self.source.read(256))
        self.size += 1
        return self.size - 1

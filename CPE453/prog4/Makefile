CC = gcc
CFLAGS = -Wall -g -pedantic -std=c99
LD = gcc
LFLAGS = -g -lm
OBJS = tinyFSDemo.o libDisk.o libTinyFS.o utilTinyFS.o
DISK_TEST_OBJS = libDisk.o diskTest.o
DISK_TEST = diskTest
UTIL_TEST_OBJS = utilTinyFS.o utilTest.o libDisk.o
UTIL_TEST = utilTest
TFS_TEST = tfsTest
TFS_TEST_OBJS = libTinyFS.o tfsTest.o utilTinyFS.o libDisk.o
TEST_EXE = utilTest diskTest
EXE = tinyFSDemo

all : tinyFSDemo

test : $(DISK_TEST_OBJS) $(UTIL_TEST_OBJS) $(TFS_TEST_OBJS)
	$(CC) $(CFLAGS) -o $(DISK_TEST) $(DISK_TEST_OBJS)
	$(CC) $(CFLAGS) -o $(UTIL_TEST) $(UTIL_TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TFS_TEST) $(TFS_TEST_OBJS)

runtest : test
	./$(DISK_TEST); echo; ./$(DISK_TEST); echo;
	./$(UTIL_TEST);
	rm *.dsk

tinyFSDemo : $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS)

tinyFSDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o : libTinyFS.c libTinyFS.h tinyFS.h libDisk.h libDisk.o tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o : libDisk.c libDisk.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

utilTinyFS.o : utilTinyFS.c utilTinyFS.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

#%.o : %.c
#	$(CC) $(CFLAGS) -c -o $@ $< -I./

.PHONY : clean
clean :
	rm *.o
	rm $(TEST_EXE) *.dsk
	rm $(EXE)

CC 	= gcc

CFLAGS  = -Wall -g -I .

LD 	= gcc

LDFLAGS  = -Wall -g 

PROGS	= snakes nums hungry

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= snakemain.c numbersmain.c hungrymain.c

HDRS	= 

EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

snakes: snakemain.o libLWP.a libsnakes.a
	$(LD) $(LDFLAGS) -o snakes snakemain.o -L. -lncurses -lsnakes -lLWP

hungry: hungrymain.o libLWP.a libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrymain.o -L. -lncurses -lsnakes -lLWP

nums: numbersmain.o libLWP.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lLWP

#Use make samples to create programs using Dr. Nico's library.
samples: sampleSnakes sampleHungry sampleNums

sampleSnakes: snakemain.o libsnakes.a
	$(LD) $(LDFLAGS) -o snakes snakemain.o -L. -lncurses -lsnakes -lPLN

sampleHungry: hungrymain.o libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrymain.o -L. -lncurses -lsnakes -lPLN

sampleNums: numbersmain.o 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lPLN

hungrymain.o: lwp.h snakes.h

snakemain.o: lwp.h snakes.h

numbermain.o: lwp.h

libLWP.a: lwp.c rr.c util.c 
	gcc -c -g rr.c util.c lwp.c magic64.S
	ar r libLWP.a util.o lwp.o rr.o magic64.o
	rm lwp.o

submission: lwp.c Makefile README
	tar -cf project2_submission.tar lwp.c Makefile README
	gzip project2_submission.tar

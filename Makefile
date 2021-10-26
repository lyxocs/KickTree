KTPATH = KickTree/

CPP=g++
CFLAGS = -g -w -std=c++14 -fpermissive -O3 $(INCLUDE)

main: main.o KickTree.o
	$(CPP) $(CFLAGS) -o main *.o $(LIBS)

# ---------------------------------------------------------------------------------------------------------
KickTree.o: $(wildcard $(KTPATH)*.cpp)
	$(CPP) $(CFLAGS) -c $(KTPATH)*.cpp

main.o: main.cpp ElementaryClasses.h
	$(CPP) $(CFLAGS) -c main.cpp

all: main
clean:
	rm -r *.o main
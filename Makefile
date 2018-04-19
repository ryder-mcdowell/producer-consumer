all: hw1

hw1: hw1.o
	g++ -pthread -o hw1 hw1.o

hw1.o: hw1.cpp
	g++ -pthread -c hw1.cpp

clean:
	rm -f hw1.o

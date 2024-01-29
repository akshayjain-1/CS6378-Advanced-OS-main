CFLAGS = -O
CC = g++ -std=c++11 

SRC = node.cc utility.cc server.cc client.cc
OBJ = $(SRC:.cc = .o)

node: $(SRC)
	$(CC) $(CFLAGS) -o node $(OBJ) -lpthread

clean:
	rm -f core *.o

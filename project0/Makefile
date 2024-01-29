CFLAGS = -O
CC = g++ -std=c++11
CLIENT_SRC = client.cc 
SERVER_SRC = server.cc
CLIENT_OBJ = $(CLIENT_SRC:.cc = .o)
SERVER_OBJ = $(SERVER_SRC:.cc = .o)

client: $(CLIENT_OBJ)
			$(CC) $(CFLAGS) -o client $(CLIENT_OBJ) 
server: $(SERVER_OBJ)
			$(CC) $(CFLAGS) -o server $(SERVER_OBJ) 

clean:
			rm -f core *.o

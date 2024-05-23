CC = g++

compile: server.cpp client.cpp
	$(CC) -std=c++11 server.cpp -pthread -o server
	$(CC) -std=c++11 client.cpp -pthread -o client
	$(CC) -std=c++11 bserver.cpp -pthread -o bserver

clean:
	rm -rf server client bserver *.txt
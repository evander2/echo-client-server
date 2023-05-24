all: echo-client echo-server

echo-client: client.cpp
	g++ -o echo-client client.cpp 

echo-server: server.cpp
	g++ -o echo-server server.cpp

clean:
	rm echo-* 

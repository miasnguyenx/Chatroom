compile:
	gcc -Wall -g3 -pthread server.c -o server
	gcc -Wall -g3 -pthread client_terminal.c -o client_terminal
	gcc -c -fPIC client.c -o client.o
	gcc -shared -Wl,-soname,client.so -o client.so  client.o

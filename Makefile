compile:
	gcc -Wall -g3 -pthread server.c -o server
	gcc -Wall -g3 -pthread client.c -o client

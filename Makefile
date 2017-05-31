CC = clang-3.7
CFLAGS = -g -Wall -std=gnu99
LDFLAGS = ''
EXES = driver_server driver_client
TESTS = test_parse


all: $(EXES)


tests: $(TESTS)


driver_server: driver_server.o shell.o raw_iterator.o packet.o server.o \
				database.o mpc.o
	$(CC) -o driver_server driver_server.o shell.o raw_iterator.o packet.o \
					server.o database.o mpc.o $(LDFLAGS)


driver_client: driver_client.o shell.o raw_iterator.o packet.o client.o \
				client_commands.o busywait.o
	$(CC) -o driver_client driver_client.o shell.o raw_iterator.o packet.o \
					client.o client_commands.o busywait.o $(LDFLAGS)


driver_client.o: driver_client.c
	$(CC) $(CFLAGS) -c driver_client.c


driver_server.o: driver_server.c
	$(CC) $(CFLAGS) -c driver_server.c


test_parse: test_parse.o mpc.o database.o
	$(CC) -o test_parse test_parse.o database.o mpc.o


test_parse.o: test_parse.c
	$(CC) $(CFLAGS) -c test_parse.c


mpc.o: mpc.h mpc.c
	$(CC) $(CFLAGS) -c mpc.c


database.o: database.h database.c
	$(CC) $(CFLAGS) -c database.c


shell.o: shell.h shell.c
	$(CC) $(CFLAGS) -c shell.c


raw_iterator.o: raw_iterator.h raw_iterator.c
	$(CC) $(CFLAGS) -c raw_iterator.c



client_commands.o: closure.h client_commands.h client_commands.c
	$(CC) $(CFLAGS) -c client_commands.c


client.o: client.h client.c
	$(CC) $(CFLAGS) -c client.c


server.o: server.h server.c
	$(CC) $(CFLAGS) -c server.c


busywait.o: busywait.h busywait.c
	$(CC) $(CFLAGS) -c busywait.c


clean:
	rm -rf *.o $(EXES) $(TESTS)


new:
	make clean
	make all


.PHONY: all new clean tests

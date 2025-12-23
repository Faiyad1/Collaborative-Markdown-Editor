CC := gcc
CFLAGS := -Wall -Wextra

all: server client

server: source/server.c markdown.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

client: source/client.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

markdown.o: source/markdown.c
	$(CC) $(CFLAGS) -c $<

clean: 
	rm -f server client markdown.o

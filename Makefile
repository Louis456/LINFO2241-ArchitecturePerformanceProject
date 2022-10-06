CC = gcc
CFLAGS += -c -std=gnu99 -Wall #-Werror -Wextra -O2
CFLAGS += -D_COLOR
LDFLAGS += -lz

CLIENT_SOURCES = $(wildcard src/client.c) #we can add more ex: src/client.c src/log.c src/packet_implem.c
SERVER_SOURCES = $(wildcard src/server.c)

CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $@ $(LDFLAGS)

$(SERVER): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean mrproper

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS)
	rm -f $(CLIENT) $(SERVER)

#tests: all
#	./tests/run_tests.sh

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += -D_DEBUG
debug: clean all
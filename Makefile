CC = gcc
CFLAGS += -c -std=gnu99 -Wall -Wextra -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2 -Werror
CFLAGS += -D_COLOR
LDFLAGS += -lz -lpthread -lm

CLIENT_SOURCES = $(wildcard src/client.c src/packet_implem.c src/utils.c src/threads.c)
SERVER_SOURCES = $(wildcard src/server.c src/packet_implem.c src/utils.c src/threads.c)

CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)

CLIENT = client
SERVER = server
SERVER_OPTIM = server-optim

all: $(CLIENT) $(SERVER) $(SERVER_OPTIM)

$(CLIENT): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $@ $(LDFLAGS)

$(SERVER): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $@ $(LDFLAGS)

$(SERVER_OPTIM): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean mrproper

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS)
	rm -f $(CLIENT) $(SERVER) $(SERVER_OPTIM)

#tests: all
#	./tests/run_tests.sh

# By default, logs are disabled. But you can enable them with the debug target.
debug: CFLAGS += -D_DEBUG
debug: clean all

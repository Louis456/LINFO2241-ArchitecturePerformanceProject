CC = gcc
CFLAGS += -c -Wall -Wextra -Werror -mno-sse2 -mno-avx -mno-avx2 -mno-avx512f -fno-unroll-loops -fno-tree-vectorize -O2
CFLAGS += -D_COLOR
LDFLAGS_CLIENT += -lpthread -lm
LDFLAGS_SERVER += -lm


CLIENT_SOURCES = $(wildcard src/client.c src/utils.c src/threads.c)
SERVER_SOURCES = $(wildcard src/server.c src/utils.c src/threads.c)

CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=client.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.c=none.o)
SERVER_OBJECTS_INLINE = $(SERVER_SOURCES:.c=inline.o)
SERVER_OBJECTS_UNROLL = $(SERVER_SOURCES:.c=unroll.o)
SERVER_OBJECTS_OPTIM = $(SERVER_SOURCES:.c=optim.o)

CLIENT = client
SERVER = server
SERVER_INLINE = server-inline
SERVER_UNROLL = server-unroll
SERVER_OPTIM = server-optim

all: $(CLIENT) $(SERVER) $(SERVER_OPTIM) $(SERVER_INLINE) $(SERVER_UNROLL)

$(CLIENT): $(CLIENT_OBJECTS)
	$(CC) $(CLIENT_OBJECTS) -o $@ $(LDFLAGS_CLIENT)

$(SERVER): $(SERVER_OBJECTS)
	$(CC) $(SERVER_OBJECTS) -o $@ $(LDFLAGS_SERVER)

$(SERVER_INLINE): $(SERVER_OBJECTS_INLINE)
	$(CC) $(SERVER_OBJECTS_INLINE) -o $@ $(LDFLAGS_SERVER)

$(SERVER_UNROLL): $(SERVER_OBJECTS_UNROLL)
	$(CC) $(SERVER_OBJECTS_UNROLL) -o $@ $(LDFLAGS_SERVER)

$(SERVER_OPTIM): $(SERVER_OBJECTS_OPTIM)
	$(CC) $(SERVER_OBJECTS_OPTIM) -o $@ $(LDFLAGS_SERVER)

%client.o: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_CLIENT)

%none.o: %.c 
	$(CC) $(CFLAGS) -D OPTIM=0 $< -o $@ $(LDFLAGS_SERVER)

%inline.o: %.c
	$(CC) $(CFLAGS) -D OPTIM=1 $< -o $@ $(LDFLAGS_SERVER)

%unroll.o: %.c
	$(CC) $(CFLAGS) -D OPTIM=2 $< -o $@ $(LDFLAGS_SERVER)

%optim.o: %.c
	$(CC) $(CFLAGS) -D OPTIM=3 $< -o $@ $(LDFLAGS_SERVER)

.PHONY: clean mrproper

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(SERVER_OBJECTS_INLINE) $(SERVER_OBJECTS_UNROLL) $(SERVER_OBJECTS_OPTIM)
	rm -f $(CLIENT) $(SERVER) $(SERVER_INLINE) $(SERVER_UNROLL) $(SERVER_OPTIM)

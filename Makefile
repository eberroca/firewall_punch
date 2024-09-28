CC=gcc
OBJS=tcp_conn.o udp.o
PROGS=client server
PROGS_OBJS=client.o server.o

all: $(PROGS)

%.o: %.c
	$(CC) -I. -c $< -o $@

client: $(OBJS) client.o
	$(CC) $(OBJS) client.o -o client

server: $(OBJS) server.o
	$(CC) $(OBJS) server.o -o server

clean:
	rm $(OBJS) $(PROGS_OBJS) $(PROGS)

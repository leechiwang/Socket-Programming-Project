CC = gcc
CFLAGS = -Wall -g

all: client serverM serverS serverD serverU

client: client.c
	$(CC) $(CFLAGS) -o client client.c

serverM: serverM.c
	$(CC) $(CFLAGS) -o serverM serverM.c

serverS: serverS.c
	$(CC) $(CFLAGS) -o serverS serverS.c

serverD: serverD.c
	$(CC) $(CFLAGS) -o serverD serverD.c

serverU: serverU.c
	$(CC) $(CFLAGS) -o serverU serverU.c

clean:
	rm -f client serverM serverS serverD serverU
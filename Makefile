# GNU Makefile
CFLAGS   := -Wall -Wextra -march=native -DNDEBUG -pthread -O3 -fopenmp
LDFLAGS  := -pthread -fopenmp
TARGET   := main

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%: %.o
	$(CC) $(LDFLAGS) $^ -o $@

all: $(TARGET)

# Dependencias
main: matriz.o main.c
matriz.o: matriz.c matriz.h

clean:
	rm -f *.o *~ $(TARGET)

CC = gcc
TARGET = rash
SRC = rash_utils.c rash_command.c
OBJ = $(SRC:.c=.o) # <- this is cool

all: $(TARGET)

tester: rash_tester

clean:
	rm rash rash_tester *.o
	
# $@ means target, $^ means dependencies
$(TARGET): rash.o $(OBJ)
	$(CC) -o $@ $^

rash_tester: rash_tester.o $(OBJ) 
	$(CC) -o $@ $^ 

# $< means the first dependency
%.o: %.c
	$(CC) -c $<
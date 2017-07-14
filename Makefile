CC = g++
CFLAGS = -g -w -std=gnu++11
ASSEMBLER_FILES = src/utils.cpp src/tables.cpp src/translate_utils.cpp src/translate.cpp
OBJ_FILES = src/utils.o src/tables.o src/translate_utils.o src/translate.o

all:
	$(CC) $(CFLAGS) -o bin/assembler src/assembler.cpp $(ASSEMBLER_FILES)

src/%.o: $(ASSEMBLER_FILES) clean
	$(CC) -o $@ -c $< $(CFLAGS)
assembler: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o src/*.o assembler

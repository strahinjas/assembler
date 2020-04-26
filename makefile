CC = g++
CFLAGS = -g -I $(SRCDIR) -std=c++11

SRCDIR = ./src
OBJDIR = ./bin/obj
TESTDIR = ./tests
TARGET = ./bin/assembler

OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp))

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TESTDIR)/*.o
	rm -f $(TESTDIR)/*.txt
	rm -f $(TARGET)

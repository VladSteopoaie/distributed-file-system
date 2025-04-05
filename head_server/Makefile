# Compiler and flags
CXX = g++
CXXFLAGS = --std=c++20 -I/usr/include/fuse3 -I/usr/include/spdlog
LDFLAGS = -lfmt -lmemcached -lfuse3 -lpthread -lprotobuf

SRC = test2.cpp

# Directories for objects and binary
OBJDIR = ../objects
BINDIR = bin
SRCDIR = ../lib

# Target executable
TARGET = $(BINDIR)/$(SRC:.cpp=)

# Source files
SRCS = cache_protocol.cpp cache_client.cpp utils.cpp metadata.pb.cpp

# Object files
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

# Default target
all: $(TARGET)
# @$(MAKE) clean

$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Link the target executable
$(TARGET): $(OBJS) $(SRC:%.cpp=$(OBJDIR)/%.o) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Allow test file to be provided as an argument
$(SRC:%.cpp=$(OBJDIR)/%.o): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR)

.PHONY: all clean

# hello: hello.c
# gcc -Wall hello.c -I/usr/include/fuse3 -lfuse3 -lpthread -o hello

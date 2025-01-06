# Compiler and flags
CXX = g++
CXXFLAGS = --std=c++20 -I/usr/include/spdlog 
LDFLAGS = -lfmt -lmemcached

TEST_SRC = test2.cpp

# Directories for objects and binary
OBJDIR = objects
BINDIR = bin
SRCDIR = lib

# Target executable
TARGET = $(BINDIR)/$(TEST_SRC:.cpp=)

# Source files
SRCS = cache_protocol.cpp cache_server.cpp cache_client.cpp cache_connection_handler.cpp utils.cpp
# SRCS = $(SRCFILES:%.cpp=$(SRCDIR)/%.cpp)

# Object files
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

# Default target
all: $(TARGET)
# @$(MAKE) clean

$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Link the target executable
$(TARGET): $(OBJS) $(TEST_SRC:%.cpp=$(OBJDIR)/%.o) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Allow test file to be provided as an argument
$(TEST_SRC:%.cpp=$(OBJDIR)/%.o): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR)

.PHONY: all clean
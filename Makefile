CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = -lpthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin

TARGET = $(BINDIR)/redis-server

SOURCES = $(shell find $(SRCDIR) -name '*.cpp')
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Extract all unique subdirectories from source files
SUBDIRS = $(sort $(dir $(SOURCES)))
OBJSUBDIRS = $(SUBDIRS:$(SRCDIR)/%=$(OBJDIR)/%)

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BINDIR)
	@mkdir -p $(OBJSUBDIRS)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

run: all
	./$(BINDIR)/redis-server

test: all
	redis-cli -p 6379 ping
CXX      = g++
CXXFLAGS = -Iinclude -Wall
LDFLAGS  = -Llib -lmingw32 -lSDL2main -lSDL2

SRC = src/main.cpp src/chip8.cpp
OUT = build/chip8

# Auto-discover test files
TEST_SOURCES := $(wildcard tests/test_*.cpp)
TEST_TARGETS := $(patsubst tests/test_%.cpp,tests/test-%,$(TEST_SOURCES))

.PHONY: all tests clean

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(OUT)
	cp lib/SDL2.dll build/

# Pattern rule: for each tests/test_*.cpp, create a tests/test-* target
tests/test-%: tests/test_%.cpp src/chip8.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/test_$*.cpp src/chip8.cpp -o build/test_$*
	./build/test_$*

# Run all tests
tests: $(TEST_TARGETS)

clean:
	rm -rf build/

# cli: g++ src/main.cpp src/chip8.cpp -Iinclude -Llib -lSDL2 -o build/chip8
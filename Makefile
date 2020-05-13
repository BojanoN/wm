OUT_FILE = wm
CXX = g++
SOURCE = $(wildcard *.cpp)
CXXFLAGS := -lX11

all:
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(OUT_FILE)

debug: CXXFLAGS += -g -DDEBUG
debug: all


clean:
	- rm -f $(OUT_FILE) *.o

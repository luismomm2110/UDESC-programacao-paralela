CC=mpicc
CXX=mpicxx
CFLAGS=-fopenmp -O3 -Wall
CXXFLAGS=-fopenmp -O3 -Wall -std=c++11
LDFLAGS=-fopenmp

TARGET=mandelbrot
TARGET_OPT=mandelbrot_otimizado
TARGET_PIPE=mandelbrot_pipeline
SOURCE=mandelbrot.cpp
SOURCE_OPT=mandelbrot_otimizado.cpp
SOURCE_PIPE=mandelbrot_pipeline.cpp

all: $(TARGET) $(TARGET_OPT) $(TARGET_PIPE)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

$(TARGET_OPT): $(SOURCE_OPT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_OPT) $(SOURCE_OPT) $(LDFLAGS)

$(TARGET_PIPE): $(SOURCE_PIPE)
	$(CXX) $(CXXFLAGS) -o $(TARGET_PIPE) $(SOURCE_PIPE) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET_OPT) $(TARGET_PIPE)

# Para executar em seu cluster heterogêneo
# Exemplo de hostfile (criar hostfile.txt):
# node1 slots=6
# node2 slots=6  
# node3 slots=6
# node4 slots=4
# node5 slots=4

run: $(TARGET)
	mpirun -np 5 --hostfile hostfile.txt --map-by ppr:1:node ./$(TARGET)

.PHONY: all clean run

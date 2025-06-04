# Makefile para Map-Reduce com MPI
CXX = mpic++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2
LDFLAGS = 

# Alvos principais
TARGETS = mpmapper mpreducer

# Regra padrão
all: $(TARGETS)

# Regras para cada executável
mpmapper: mpmapper.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

mpreducer: mpreducer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Executar com diferentes números de processos
run-2: mpreducer
	mpirun -np 2 ./mpreducer

run-4: mpreducer
	mpirun -np 4 ./mpreducer

run-8: mpreducer
	mpirun -np 8 ./mpreducer

# Executar mapper
run-mapper-2: mpmapper
	mpirun -np 2 ./mpmapper

run-mapper-4: mpmapper
	mpirun -np 4 ./mpmapper

# Debug build
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(TARGETS)

# Limpar arquivos compilados
clean:
	rm -f $(TARGETS) *.o

# Instalar dependências MPI (para sistemas baseados em Debian/Ubuntu)
install-deps:
	sudo apt-get update
	sudo apt-get install -y openmpi-bin openmpi-common libopenmpi-dev

# Verificar instalação MPI
check-mpi:
	@echo "Verificando instalação MPI..."
	@which mpirun || echo "mpirun não encontrado!"
	@which mpic++ || echo "mpic++ não encontrado!"
	@mpirun --version 2>/dev/null || echo "Erro ao executar mpirun!"

# Mostrar ajuda
help:
	@echo "Alvos disponíveis:"
	@echo "  all          - Compilar todos os programas"
	@echo "  mpmapper     - Compilar apenas o mapper"
	@echo "  mpreducer    - Compilar apenas o reducer"
	@echo "  run-2        - Executar reducer com 2 processos"
	@echo "  run-4        - Executar reducer com 4 processos"
	@echo "  run-8        - Executar reducer com 8 processos"
	@echo "  run-mapper-2 - Executar mapper com 2 processos"
	@echo "  run-mapper-4 - Executar mapper com 4 processos"
	@echo "  debug        - Compilar com informações de debug"
	@echo "  clean        - Limpar arquivos compilados"
	@echo "  install-deps - Instalar dependências MPI"
	@echo "  check-mpi    - Verificar instalação MPI"
	@echo "  help         - Mostrar esta ajuda"

.PHONY: all clean debug run-2 run-4 run-8 run-mapper-2 run-mapper-4 install-deps check-mpi help 
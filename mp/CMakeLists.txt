cmake_minimum_required(VERSION 3.10)
project(MapReduceMPI)

# Configurar C++14 como padrão
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar MPI
find_package(MPI REQUIRED)

# Verificar se MPI foi encontrado
if(MPI_FOUND)
    message(STATUS "MPI encontrado!")
    message(STATUS "MPI include directories: ${MPI_INCLUDE_PATH}")
    message(STATUS "MPI libraries: ${MPI_LIBRARIES}")
else()
    message(FATAL_ERROR "MPI não foi encontrado!")
endif()

# Incluir diretórios MPI
include_directories(${MPI_INCLUDE_PATH})

# Criar executável para o mapper
add_executable(mpmapper mpmapper.cpp)
target_link_libraries(mpmapper ${MPI_LIBRARIES})

# Adicionar flags de compilação MPI para o mapper
if(MPI_COMPILE_FLAGS)
    set_target_properties(mpmapper PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(mpmapper PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

# Criar executável para o reducer
add_executable(mpreducer mpreducer.cpp)
target_link_libraries(mpreducer ${MPI_LIBRARIES})

# Adicionar flags de compilação MPI para o reducer
if(MPI_COMPILE_FLAGS)
    set_target_properties(mpreducer PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(mpreducer PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

# Flags de debug e otimização
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Definir tipo de build padrão
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

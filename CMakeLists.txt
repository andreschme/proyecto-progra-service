cmake_minimum_required(VERSION 3.28)
project(proyecto-progra-service)

set(CMAKE_CXX_STANDARD 17)

add_executable(proyecto-progra-service main.cpp
        librerias/httplib.h
        librerias/json.hpp
        librerias/Alumno.h
)

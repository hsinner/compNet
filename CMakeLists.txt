cmake_minimum_required(VERSION 3.31.5)
project(compNetRepo C)

set(CMAKE_C_STANDARD 11)

add_executable(compNetRepo
    Lab00/list.c
    Lab00/paint.c
    Lab00/palindrome.c
    Lab01/client.c
    Lab01/server.c
    Lab02/clientDir/client.c
    Lab02/serverDir/server.c
    Lab03/proxy.c)

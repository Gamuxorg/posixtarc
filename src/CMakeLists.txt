set(SOURCES
        tar.h
        TarStream.c
        TarStream.h)

add_library(gamux-tar SHARED ${SOURCES})
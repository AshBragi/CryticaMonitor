cmake_minimum_required(VERSION 3.16)
project(CryticaMonitor C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(CryticaMonitor main.c csl_constants.h csl_message.h
        csl_mysql.h csl_crypto.h csl_message.c csl_utilities.c csl_utilities.h
        csl_mysql.c CryticaMonitor.h csl_crypto.c)

target_link_libraries(CryticaMonitor
        /usr/lib/x86_64-linux-gnu/libmysqlclient.so.20
        /usr/lib/x86_64-linux-gnu/libczmq.so.4
        /usr/lib/x86_64-linux-gnu/libzmq.so.5
        /usr/lib/x86_64-linux-gnu/libcrypto.so.1.1
        /usr/lib/x86_64-linux-gnu/libssl.so.1.1)

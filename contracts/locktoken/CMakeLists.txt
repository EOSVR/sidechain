cmake_minimum_required(VERSION 3.10)
project(locktoken)

set(CMAKE_CXX_STANDARD 11)

include_directories(.
        /usr/local/Cellar/boost/1.67.0_1/include
        /Users/dacheng/eos/eosio.cdt/libraries
        /Users/dacheng/eos/eosio.cdt/build/include
        )

add_executable(locktoken
        locktoken.cpp
        locktoken.hpp)

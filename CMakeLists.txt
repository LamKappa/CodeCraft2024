cmake_minimum_required(VERSION 3.8)
project(CodeCraftSDK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
link_libraries(pthread rt m)

AUX_SOURCE_DIRECTORY(. src)
ADD_EXECUTABLE(main ${src}
        Mission.cpp)
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(kai2)

add_library (kai2 src/kaiFunction.cpp src/kaiBuffer.cpp src/kaiIDGenerator.cpp src/kaiServer.cpp src/kaiMsg.cpp src/kai.cpp src/kaiSocket.cpp)
#target_link_libraries (test_opencv ${OpenCV_LIBS})


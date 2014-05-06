add_definitions(-std=c++11)
add_executable(server server.cpp)
target_link_libraries(server boost_system boost_thread)
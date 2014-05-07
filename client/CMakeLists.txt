add_definitions(-std=c++11)

include (${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

qt4_wrap_ui(client_ui client.ui)

set(CMAKE_AUTOMOC ON)
add_executable(client client.cpp ${client_ui})
target_link_libraries(client ${QT_LIBRARIES} boost_system boost_thread)

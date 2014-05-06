project(pony)

add_executable(pony main.cpp)

install(TARGETS pony RUNTIME DESTINATION bin)

add_subdirectory(server)
add_subdirectory(client)
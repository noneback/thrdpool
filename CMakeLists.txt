cmake_minimum_required(VERSION 3.12)
project(thrdpool_example)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

include_directories("src/thrdpool")

# add_subdirectory(
# "src/thrdpool"
# )
add_executable(${PROJECT_NAME} src/main.cc)

# target_link_libraries(${PROJECT_NAME} libthrdpool)

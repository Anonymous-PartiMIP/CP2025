cmake_minimum_required(VERSION 3.15)

project(PartiMIP-HiGHS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(HIGHS_DIR ../BaseSolver/HiGHS/HiGHS-1.9.0/lib/cmake/highs)

find_package(HIGHS REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

set(INCLUDES ${PROJECT_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(${INCLUDES})

target_link_libraries(PartiMIP-HiGHS highs::highs pthread boost_thread boost_date_time boost_system)
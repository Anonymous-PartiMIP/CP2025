cmake_minimum_required(VERSION 3.15)

project(PartiMIP-SCIP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

set(HIGHS_DIR ../BaseSolver/HiGHS/HiGHS-1.10.0)
set(SCIP_DIR ../BaseSolver/SCIP/SCIPOptSuite-9.2.0-Linux)

include_directories(${HIGHS_DIR}/include)
include_directories(${HIGHS_DIR}/include/highs)

link_directories(${HIGHS_DIR}/lib)

include_directories(${SCIP_DIR}/include)
link_directories(${SCIP_DIR}/lib)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

set(INCLUDES ${PROJECT_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(${INCLUDES})

target_link_libraries(PartiMIP-SCIP 
    highs
    scip
    pthread 
    boost_thread 
    boost_date_time 
    boost_system)

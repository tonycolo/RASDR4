cmake_minimum_required(VERSION 3.8)
project(rasdr4)

MACRO( FIND_PATH_REQUIRED arg1 arg2 )
    FIND_PATH( ${arg1} ${arg2} ${ARGN} )
    IF( "${${arg1}}" STREQUAL "${arg1}-NOTFOUND"  )
        MESSAGE( FATAL_ERROR "${arg2} not found" )
    ELSE()
        MESSAGE( STATUS "${arg2} found in ${${arg1}}" )
    ENDIF()
ENDMACRO( FIND_PATH_REQUIRED )

set(CMAKE_CXX_STANDARD 11)
set(USE_GNU_PLOT)
add_definitions(-DUSE_GNU_PLOT)
set (CMAKE_CXX_FLAGS "-fPIC")
set (CMAKE_AUTOMOC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

SET(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,$ORIGIN")

link_directories(${CMAKE_SOURCE_DIR})
message(${CMAKE_SOURCE_DIR})
include_directories( "sw/rtaudio" "sw/poco-1.9.0/Util/include" "sw/poco-1.9.0/Foundation/include")
set_source_files_properties(PROPERTIES CPLUSPLUS on)

file(GLOB rasdr4_srcs src/*.h src/*.cpp)
set(SRCS ${rasdr4_srcs})

add_executable(rasdr4 ${SRCS})
target_link_libraries(rasdr4 LimeSuite fftw3 fftw3_threads pthread rtaudio PocoJSON PocoXML PocoFoundation PocoUtil)

#scratch section
#find_library(LIMESUITE NAMES LimeSuite HINTS "/home/astro/Documents/LimeSuite18.04.1/LimeSuite-18.04.1/build2/src")
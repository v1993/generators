set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Debug)

cmake_minimum_required(VERSION 3.1)
project(Generators)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(COMMON_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_compile_options(-Ofast) # For best results

add_subdirectory(main) # Loader
add_subdirectory(test) # Test backend
add_subdirectory(markov) # Markov chain backnd (usable one)

add_subdirectory(markovSql)
add_subdirectory(markovSQLClient)

add_subdirectory(markovSQLite3)
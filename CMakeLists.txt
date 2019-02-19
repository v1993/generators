set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.1)
project(Generators)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)

set(BUILD_STATIC FALSE CACHE BOOL "Use static linkage for boost")

if (BUILD_STATIC)
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_STATIC_RUNTIME    ON)
else()
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_STATIC_RUNTIME    OFF)
endif()

set(Boost_USE_MULTITHREADED     ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(COMMON_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(main) # Loader
add_subdirectory(test) # Test backend
add_subdirectory(markov) # Markov chain backnd (usable one)

add_subdirectory(markovSql)
add_subdirectory(markovSQLClient)

add_subdirectory(markovSQLite3)

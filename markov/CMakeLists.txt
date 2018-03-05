set (CMAKE_CXX_STANDARD 17)
add_library(markovBackend SHARED
	markov.cpp
	cache.cpp
)

set( BOOST_COMPONENTS_NEEDED regex filesystem program_options serialization random )
FIND_PACKAGE(Boost 1.66.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)

target_include_directories(markovBackend PRIVATE ${COMMON_INCLUDES} ${Boost_INCLUDE_DIRS})
target_link_libraries(markovBackend ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(markovBackend PROPERTIES LINK_FLAGS -fvisibility=hidden)

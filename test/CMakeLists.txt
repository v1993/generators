set (CMAKE_CXX_STANDARD 17)
add_library(testBackend SHARED
	test.cpp
)

target_include_directories(testBackend PRIVATE ${COMMON_INCLUDES})
target_link_libraries(testBackend ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(testBackend PROPERTIES LINK_FLAGS -fvisibility=hidden)

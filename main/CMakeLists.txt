set (CMAKE_CXX_STANDARD 17) # Yeeeeeeeeah! (It is 2018 now, so not 20 :( )
add_executable(generators
	# List of .cpp's
	Main.cpp
	Parseopts.cpp
)

set( BOOST_COMPONENTS_NEEDED thread filesystem program_options )
FIND_PACKAGE(Boost 1.66.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)

target_include_directories(generators PRIVATE ${COMMON_INCLUDES} ${Boost_INCLUDE_DIRS})
target_link_libraries(generators ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

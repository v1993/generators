set (CMAKE_CXX_STANDARD 17)
add_executable(markovSQLClient
	main.cpp
)

set( BOOST_COMPONENTS_NEEDED program_options regex filesystem )
FIND_PACKAGE(Boost 1.52.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)

target_include_directories(markovSQLClient PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(markovSQLClient ${Boost_LIBRARIES})

target_link_libraries(markovSQLClient /usr/lib/libmysqlcppconn.so)

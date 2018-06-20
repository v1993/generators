set (CMAKE_CXX_STANDARD 17)
add_executable(markovSQLClient
	main.cpp
)

set( BOOST_COMPONENTS_NEEDED regex filesystem program_options )
FIND_PACKAGE(Boost 1.52.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)
FIND_PACKAGE(MysqlCppConn REQUIRED)

target_include_directories(markovSQLClient PRIVATE ${Boost_INCLUDE_DIRS} ${MYSQLCONNECTORCPP_INCLUDE_DIRS})
target_link_libraries(markovSQLClient ${Boost_LIBRARIES} ${MYSQLCONNECTORCPP_LIBRARIES})

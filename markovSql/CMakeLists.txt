set (CMAKE_CXX_STANDARD 17)
add_library(markovSqlBackend SHARED
	markov.cpp
	cache.cpp
)

set( BOOST_COMPONENTS_NEEDED regex filesystem program_options )
FIND_PACKAGE(Boost 1.66.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)
FIND_PACKAGE(MysqlCppConn REQUIRED)

target_include_directories(markovSqlBackend PRIVATE ${COMMON_INCLUDES} ${Boost_INCLUDE_DIRS} ${MYSQLCONNECTORCPP_INCLUDE_DIRS})
target_link_libraries(markovSqlBackend ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${MYSQLCONNECTORCPP_LIBRARIES})

set_target_properties(markovSqlBackend PROPERTIES LINK_FLAGS -fvisibility=hidden)

set (CMAKE_CXX_STANDARD 17)
add_library(markovSQLite3Backend SHARED
	markov.cpp
	cache.cpp
)

set( BOOST_COMPONENTS_NEEDED thread regex filesystem program_options )
FIND_PACKAGE(Boost 1.66.0 COMPONENTS ${BOOST_COMPONENTS_NEEDED} REQUIRED)
FIND_PACKAGE(SQLite3 REQUIRED)

target_include_directories(markovSQLite3Backend PRIVATE ${COMMON_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/sqlite_modern_cpp/hdr)
target_link_libraries(markovSQLite3Backend
Boost::thread
Boost::regex
Boost::filesystem
Boost::program_options
# ${CMAKE_THREAD_LIBS_INIT}
SQLite::SQLite3)

set_target_properties(markovSQLite3Backend PROPERTIES LINK_FLAGS -fvisibility=hidden)

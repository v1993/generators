#include <generatorAPI.hpp>

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <deque>
#include <utility>
#include <boost/regex.hpp>

template <typename Container> // we can make this generic for any container
struct container_hash {
    std::size_t operator()(Container const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};

namespace markov {
	using MarkovDeque = std::deque<std::string>;
	template<typename T>
	void shiftDeque(std::deque<T>&, T&);
	class markovBackend: public generatorAPI { // We store most info inside SQL or class
		public:
			void init(std::vector<std::string>);
			boost::any train(std::shared_ptr<std::ifstream>);
			boost::any merge(std::vector<boost::any>&);
			void out(boost::any&, std::shared_ptr<std::ostream>);

			boost::any load(std::string);
			void save(std::string, boost::any&);

		protected:
			void trainPart(std::string, const std::unique_ptr<sql::Connection>&, const std::unique_ptr<sql::PreparedStatement>&);
			void trainFinal(std::string, const std::unique_ptr<sql::Connection>&, const std::unique_ptr<sql::PreparedStatement>&, MarkovDeque&);
			void trainInsert(std::string, const std::unique_ptr<sql::Connection>&, const std::unique_ptr<sql::PreparedStatement>&, MarkovDeque&);

			void addIdx(const std::unique_ptr<sql::Connection>&);

			std::unique_ptr<sql::Connection> mysql_connect();

			std::string outGet(const std::unique_ptr<sql::Connection>&, const std::unique_ptr<sql::PreparedStatement>&, MarkovDeque&);

			std::string iter;
			std::string prefixmiddle;
			unsigned int N;
			bool splitstr;
			unsigned long long int maxgen;
			bool rndstart;
			boost::regex separator;

			boost::sregex_token_iterator xInvalidTokenIt;
			boost::sregex_iterator xInvalidIt;

			std::string mysql_endpoint;
			std::string mysql_user;
			std::string mysql_passwd;
			std::string mysql_database;
			std::string mysql_table;
			unsigned int mysql_index;
			bool mysql_transactions;

			sql::Driver *driver;
			std::string mysql_insert; // Template for INSERT
			std::string mysql_query; // Template for strings query
	};
}

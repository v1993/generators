#pragma once
#include <generatorAPI.hpp>

#include <sqlite_modern_cpp.h>

#include <deque>
#include <utility>
#include <boost/regex.hpp>
#include <mutex>

template <typename Container> // we can make this generic for any container
struct container_hash {
	std::size_t operator()(Container const& c) const {
		return boost::hash_range(c.begin(), c.end());
		}
	};

namespace markov {
	using rowid_t = long long;
	using MarkovDeque = std::deque<rowid_t>;
	template<typename T>
	void shiftDeque(std::deque<T>&, T&);
	class markovBackend: public generatorAPI { // We store most info inside SQL or class
		public:
			void init(std::vector<std::string>);
			void trainBegin(std::vector<std::shared_ptr<std::ifstream>>);
			boost::any train(std::shared_ptr<std::ifstream>);
			boost::any merge(std::vector<boost::any>&);
			void out(boost::any&, std::shared_ptr<std::ostream>);

			boost::any load(std::string);
			void save(std::string fname, boost::any& data);

		private:
			std::mutex mutex;
			void trainFile(std::shared_ptr<std::ifstream>, std::function<void(const std::string&, const bool&)>);
			void trainPart(const std::string&, std::function<void(const std::string&, const bool&)>);
			void trainFinal(const std::string&, std::function<void(const std::string&, const bool&)>);
			void trainInsert(const std::string&, sqlite::database, sqlite::database_binder&, MarkovDeque&);

			std::unique_ptr<sqlite::database> connect();
			std::unique_ptr<sqlite::database> mainConnection;

			rowid_t outGet(sqlite::database_binder&, MarkovDeque&);

			std::string iter;
			std::string prefixmiddle;
			unsigned int N;
			bool splitstr;
			unsigned long long int maxgen;
			bool rndstart;
			boost::regex separator;

			boost::sregex_token_iterator xInvalidTokenIt;
			boost::sregex_iterator xInvalidIt;

			std::string database_uri; // By default, use in-memory database
			std::string sqlite_table;
			std::string sqlite_table_dict;
			bool sqlite_index;

			rowid_t dictID(const std::string&, sqlite::database&);
			std::string dictStr(const rowid_t&, sqlite::database&);

			std::string sqlite_insert; // Template for INSERT
			std::string sqlite_insert_dict; // Template for INSERT (to dict)
			std::string sqlite_query; // Template for strings query
			
			std::string dict_idx_name;
			std::string table_idx_name;
		};
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

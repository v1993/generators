#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <deque>
#include <utility>
#include <streambuf>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

using MarkovDeque = std::deque<std::string>;
namespace po = boost::program_options;

class markovBackend {
	public:
		void init(std::string);
		void out();
		std::string outGet(const std::unique_ptr<sql::PreparedStatement>&, MarkovDeque&);
	protected:
		std::string iter;
		std::string prefixmiddle;
		unsigned int N;
		bool splitstr;
		unsigned long long int maxgen;
		bool rndstart;
		boost::regex separator;

		std::string mysql_endpoint;
		std::string mysql_user;
		std::string mysql_passwd;
		std::string mysql_database;
		std::string mysql_table;
		unsigned int mysql_index;
		bool mysql_transactions;

		std::string mysql_query; // Template for strings query
};

template<typename T>
void shiftDeque(std::deque<T> &dq, T &elem) {
	dq.pop_front();
	dq.push_back(elem);
};

static void checkFile(std::string fname) {
	if (!boost::filesystem::exists(fname)) {
		throw std::invalid_argument("file `"+fname+"` not found.");
	}
}

boost::smatch mathConfigStr(std::string param, std::string str, boost::regex regex) {
	boost::smatch res;
	if (!boost::regex_match(str, res, regex)) {
		throw std::invalid_argument("Malformed config param: "+param);
	}
	return res;
}

std::string repeatDelim(std::string str, std::string delim, unsigned int cnt) { // Correct for cnt > 0
	std::string join = str + delim; // Cache
	std::string res;
	res.reserve(str.length()*cnt+delim.length()*(cnt-1));
	for(unsigned int i = 1; i < cnt; i++) {
		res+=join;
	}
	res+=str;
	return res;
}

std::string joinStr(std::vector<std::string>& v, std::string delim) {
	std::stringstream ss;
		for(size_t i = 0; i < v.size(); ++i) {
			if(i != 0) ss << delim;
			ss << v[i];
		}
	return ss.str();
}

std::string configString(std::string param, po::variables_map vm) {
	std::string str = mathConfigStr(param, vm[param].as<std::string>(), boost::regex("\"(.*)\""))[1]; // String must be in ""
	auto fail = [param](){throw std::invalid_argument("Invalid escape sequence in config param "+param);};
	auto it = str.begin();
	std::string res;
	while (it != str.end()) {
		char c = *it++;
		if (c == '\\' and it != str.end()) {
			char c2 = *it++;
			switch (c2) {
				case '\\': c = '\\'; break;
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
			default: // All advanced cases
				if (c2 >= '0' and c2 <= '7' and it != str.end()) { // 8-based number
					char c3 = *it++;
					if (c3 >= '0' and c3 <= '7' and it != str.end()) {
						char c4 = *it++;
						if (c4 >= '0' and c4 <= '7') {
							const char a[4] = {c2, c3, c4, '\0'};
							c = strtol(a, NULL, 8);
						} else {
							fail();
						}
					} else if (c2 == '0') {
						c = '\0';
					} else {
						fail();
					}
				} else if (c2 == 'x' and it != str.end()) {
					auto checkHex = [](char c){ return ((c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F')); };
					char c3 = *it++;
					if (checkHex(c3) and it != str.end()) {
						char c4 = *it++;
						if (checkHex(c4)) {
							char a[3] = {c3, c4, '\0'};
							c = strtol(a, NULL, 16);
						} else {
							fail();
						}
					} else {
						fail();
					}
				} else {
					fail();
				}
			}
		}
		res += c;
	}
	return res;
}

void markovBackend::init(std::string fname) { // TODO: add more foolproof
	po::options_description config("Config file");
	config.add_options()
		("iter", po::value<std::string>()->required(), "iterator (to select block, regex)")
		("prefixmiddle", po::value<std::string>()->required(), "insert between captured patterns")
		("N", po::value<unsigned int>()->required(), "consider N captures")
		("splitstr", po::value<bool>()->required(), "parse string by string, not all file")
		("maxgen", po::value<unsigned long long int>()->required(), "do not print more than maxgen block  (zero to no limit)")
		("rndstart", po::value<bool>()->default_value(false, "false"), "start from random combination")
		("separator", po::value<std::string>(), "block separator (regex)")
		("mysql_endpoint", po::value<std::string>()->required(), "path to mySQL endpoint, e.g. \"tcp://127.0.0.1:3306\"")
		("mysql_user", po::value<std::string>()->required(), "mySQL user")
		("mysql_passwd", po::value<std::string>()->required(), "mySQL password")
		("mysql_database", po::value<std::string>()->required(), "mySQL database")
		("mysql_table", po::value<std::string>()->required(), "mySQL table")
		("mysql_index", po::value<unsigned int>()->default_value(0), "count of indexed chars (no or 0 means no index, which pretty bad)")
		("mysql_transactions", po::value<bool>()->default_value(false), "use transactions or not (NOTE: true for 100.000+ blocks per file is good)");
	po::variables_map vm;
	{
		checkFile(fname);
		std::ifstream file;
		file.open(fname);
		store(parse_config_file(file, config), vm);
		file.close();
	}
	notify(vm);
	prefixmiddle = configString("prefixmiddle", vm);
	N = vm["N"].as<unsigned int>();
	maxgen = vm["maxgen"].as<unsigned long long int>();
	rndstart = vm["rndstart"].as<bool>();

	mysql_endpoint = configString("mysql_endpoint", vm);
	mysql_user = configString("mysql_user", vm);
	mysql_passwd = configString("mysql_passwd", vm);
	mysql_database = configString("mysql_database", vm);
	mysql_table = configString("mysql_table", vm);

	{
		std::vector<std::string> vec;
		for (int i = 1; i <= N; i++) {
			vec.push_back("str"+std::to_string(i)+"=?"); // All "base" strings
		}
		mysql_query = "SELECT rstr FROM "+mysql_table+" WHERE "+joinStr(vec, " AND ")+" ORDER BY RAND() LIMIT 1;";
	}
};

void markovBackend::out() {
	sql::Driver *driver = get_driver_instance();
	std::unique_ptr<sql::Connection> con(driver->connect(mysql_endpoint, mysql_user, mysql_passwd));
	con->setSchema(mysql_database);
	{
		std::unique_ptr<sql::Statement> stm(con->createStatement());
		stm->execute("set character set utf8mb4");
	}
	std::unique_ptr<sql::PreparedStatement> pstm(con->prepareStatement(mysql_query));
	MarkovDeque dq;
	if (rndstart) {
		dq = MarkovDeque(N);
		std::unique_ptr<sql::Statement> stm(con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stm->executeQuery("SELECT * FROM "+mysql_table+" ORDER BY RAND() LIMIT 1;")); // Select one field
		res->next();
		for (int i = 1; i <= N; i++) {
			dq[i-1] = res->getString(i+1);
		}
	} else {
		dq = MarkovDeque(N, "");
	}
	std::string str = outGet(pstm, dq);
	unsigned long long int n = 0;
	while(str != "") {
		std::cout << str;
		if (str != "\n") { std::cout << prefixmiddle; };
		if (maxgen > 0) {
			if (++n == maxgen) { return; }; // We reached limit
		}
		shiftDeque(dq, str);
		str = outGet(pstm, dq);
	}
};

std::string markovBackend::outGet(const std::unique_ptr<sql::PreparedStatement>& pstm, MarkovDeque& dq) {
	for (int i = 1; i <= N; i++) {
		pstm->setString(i, dq.at(i-1));
	}
	std::unique_ptr<sql::ResultSet> res(pstm->executeQuery());
	if (res->rowsCount() == 0) { return ""; } // Not found at all, hopeless
	else { res->next(); return res->getString(1); } // Return match
};

int main(int  ac, char* av[]) {
	if (ac != 2) { std::cerr << "Wrong arguments" << std::endl; return 1; };
	markovBackend b;
	b.init(av[1]);
	b.out();
	return 0;
}

#include "interface.hpp"
#include <streambuf>
#include <algorithm>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;

namespace markov {
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

	std::unique_ptr<sql::Connection> markovBackend::mysql_connect() {
		std::unique_ptr<sql::Connection> con(driver->connect(mysql_endpoint, mysql_user, mysql_passwd));
		con->setSchema(mysql_database);
		std::unique_ptr<sql::Statement> stm(con->createStatement());
		stm->execute("set character set utf8mb4");
		return con;
	}

	void markovBackend::init(std::vector<std::string> opts) { // TODO: add more foolproof
		if (opts.size() != 1) {throw std::invalid_argument("You should give only config file name to markov backend (via backend-opts) or helpme to display help");};
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
		if (opts.front() == "helpme") {
			std::cout << "Due to restrictions of boost::program_options, options prints in cmd format." << std::endl
			<< "Actual format is ini-like `opt=val`. All strings must be placed into \"\" and most of C escapes (`\\n` for example) will be applied to them."
			<< config << std::endl;
			exit(EXIT_SUCCESS);
		};
		po::variables_map vm;
		{
			checkFile(opts.front());
			std::ifstream file;
			file.open(opts.front());
			store(parse_config_file(file, config), vm);
			file.close();
		}
		notify(vm);
		iter = configString("iter", vm);
		prefixmiddle = configString("prefixmiddle", vm);
		N = vm["N"].as<unsigned int>();
		splitstr = vm["splitstr"].as<bool>();
		maxgen = vm["maxgen"].as<unsigned long long int>();
		rndstart = vm["rndstart"].as<bool>();
		separator = vm.count("separator") ? configString("separator", vm) : "";

		mysql_endpoint = configString("mysql_endpoint", vm);
		mysql_user = configString("mysql_user", vm);
		mysql_passwd = configString("mysql_passwd", vm);
		mysql_database = configString("mysql_database", vm);
		mysql_table = configString("mysql_table", vm);
		mysql_index = vm["mysql_index"].as<unsigned int>();
		mysql_transactions = vm["mysql_transactions"].as<bool>();

		driver = get_driver_instance();
		{
			std::unique_ptr<sql::Connection> con = mysql_connect();
			std::cout << "Connect OK!" << std::endl;
			con->setSchema(mysql_database);
			std::unique_ptr<sql::Statement> stm(con->createStatement());
			std::unique_ptr<sql::ResultSet> res(stm->executeQuery("SHOW TABLES LIKE \""+mysql_table+"\";"));
			std::cout << "Show tables ok!" << std::endl;
			if (res->rowsCount() == 0) { // Table does not exist, create it
				std::vector<std::string> vec = {//"id BIGINT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT",
					"rstr TEXT NOT NULL" }; // Result str
				for (int i = 1; i <= N; i++) {
					vec.push_back("str"+std::to_string(i)+" TEXT NOT NULL"); // All "base" strings
				}
				std::unique_ptr<sql::Statement> stm(con->createStatement());
				stm->execute("CREATE TABLE "+mysql_table+" ("+joinStr(vec, ", ")+") ENGINE=InnoDB CHARACTER SET=utf8mb4 COLLATE=utf8mb4_bin;");
				addIdx(con);
			}
		}
		{
			std::vector<std::string> vec;
			std::vector<std::string> vec2 = {"rstr"};
			for (int i = 1; i <= N; i++) {
				vec.push_back("str"+std::to_string(i)+"=?"); // All "base" strings
				vec2.push_back("str"+std::to_string(i));
			}
			mysql_query = "SELECT rstr FROM "+mysql_table+" WHERE "+joinStr(vec, " AND ")+" ORDER BY RAND() LIMIT 1;";
			mysql_insert = "INSERT INTO "+mysql_table+"("+joinStr(vec2, ", ")+") VALUES("+repeatDelim("?", ", ", N+1)+");";
		}
	};

	void markovBackend::trainPart(std::string data, const std::unique_ptr<sql::Connection>& con, const std::unique_ptr<sql::PreparedStatement>& pstm) {
		MarkovDeque dq(N, "");
		if (splitstr) {
			boost::sregex_token_iterator linesIter(data.begin(), data.end(), boost::regex("\n+"), -1);
			while(linesIter != xInvalidTokenIt) {
				trainFinal(*linesIter++, con, pstm, dq);
				trainInsert("\n", con, pstm, dq);
			}
		} else {
			trainFinal(data, con, pstm, dq);
		}
		trainInsert("", con, pstm, dq); // Insert end*/
	};

	void markovBackend::trainFinal(std::string data, const std::unique_ptr<sql::Connection>& con, const std::unique_ptr<sql::PreparedStatement>& pstm, MarkovDeque& dq) {
		boost::sregex_iterator blocksIter(data.begin(), data.end(), boost::regex(iter));
		while (blocksIter != xInvalidIt) {
			trainInsert((blocksIter++)->str(), con, pstm, dq);
		}
	};

	void markovBackend::trainInsert(std::string data, const std::unique_ptr<sql::Connection>& con, const std::unique_ptr<sql::PreparedStatement>& pstm, MarkovDeque& dq) {
		pstm->setString(1, data);
		for (int i = 1; i <= N; i++) {
			pstm->setString(i+1, dq.at(i-1));
		}
		pstm->execute();
		shiftDeque<std::string>(dq, data);
	}

	boost::any markovBackend::train(std::shared_ptr<std::ifstream> file) {
		driver->threadInit();
		{
			std::unique_ptr<sql::Connection> con = mysql_connect();
			std::unique_ptr<sql::Statement> stm(con->createStatement());
			stm->execute("SET AUTOCOMMIT=0;");
			if (mysql_transactions) {
				stm->execute("SET TRANSACTION ISOLATION LEVEL READ COMMITTED;");
			}
			stm->execute("START TRANSACTION;");
			std::unique_ptr<sql::PreparedStatement> pstm(con->prepareStatement(mysql_insert));
			std::string content;
			{
				file->seekg(0, std::ios::end);   
				content.reserve(file->tellg());
				file->seekg(0, std::ios::beg);

				content.assign((std::istreambuf_iterator<char>(*file)),
								std::istreambuf_iterator<char>());
				file->close();
			}
			{
				if (separator != boost::regex("")) {
					boost::sregex_token_iterator partsIter(content.begin(), content.end(), separator, -1);
					while(partsIter != xInvalidTokenIt) {
						trainPart(*partsIter++, con, pstm);
					}
				} else {
					trainPart(content, con, pstm);
				}
			}
			stm->execute("COMMIT;");
		}
		driver->threadEnd();
		return true;
	};

	boost::any markovBackend::merge(std::vector<boost::any>&) {
		return true;
	};

	void markovBackend::addIdx(const std::unique_ptr<sql::Connection>& con) {
		if (mysql_index > 0) { // Build index by `mysql_index` chars
			std::vector<std::string> vec;
			for (int i = 1; i <= N; i++) {
				vec.push_back("str"+std::to_string(i)+"("+std::to_string(mysql_index)+")"); // All "base" strings
			}
			std::unique_ptr<sql::Statement> stm(con->createStatement());
			std::cout << "Creating index, it may took some time... ";
			stm->execute("CREATE INDEX markov ON "+mysql_table+"("+joinStr(vec, ", ")+");");
			std::cout << "done!" << std::endl;
		}
	}

	void markovBackend::out(boost::any&, std::shared_ptr<std::ostream> o) {
		std::unique_ptr<sql::Connection> con = mysql_connect();
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
		std::string str = outGet(con, pstm, dq);
		unsigned long long int n = 0;
		while(str != "") {
			(*o) << str;
			if (str != "\n") { (*o) << prefixmiddle; };
			if (maxgen > 0) {
				if (++n == maxgen) { return; }; // We reached limit
			}
			shiftDeque(dq, str);
			str = outGet(con, pstm, dq);
		}
	};

	std::string markovBackend::outGet(const std::unique_ptr<sql::Connection>&, const std::unique_ptr<sql::PreparedStatement>& pstm, MarkovDeque& dq) {
		for (int i = 1; i <= N; i++) {
			pstm->setString(i, dq.at(i-1));
		}
		std::unique_ptr<sql::ResultSet> res(pstm->executeQuery());
		if (res->rowsCount() == 0) { return ""; } // Not found at all, hopeless
		else { res->next(); return res->getString(1); } // Return match
	};

	markovBackend backendInterface;
}

BOOST_DLL_ALIAS(markov::backendInterface, backendInterface) // Export variable

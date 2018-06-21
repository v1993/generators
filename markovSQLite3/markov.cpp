#include "interface.hpp"
#include <streambuf>
#include <algorithm>
#include <chrono>
#include <thread>
#include <boost/dll/alias.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

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
		for (unsigned int i = 1; i < cnt; i++) {
				res+=join;
				}
		res+=str;
		return res;
		}

	std::string joinStr(std::vector<std::string>& v, std::string delim) {
		std::stringstream ss;
		for (size_t i = 0; i < v.size(); ++i) {
				if (i != 0) ss << delim;
				ss << v[i];
				}
		return ss.str();
		}

	std::string configString(std::string param, po::variables_map vm) {
		std::string str = mathConfigStr(param, vm[param].as<std::string>(), boost::regex("\"(.*)\""))[1]; // String must be in ""
		auto fail = [param]() {
			throw std::invalid_argument("Invalid escape sequence in config param "+param);
			};
		auto it = str.begin();
		std::string res;
		while (it != str.end()) {
				char c = *it++;
				if (c == '\\' and it != str.end()) {
						char c2 = *it++;
						switch (c2) {
								case '\\':
									c = '\\';
									break;
								case 'a':
									c = '\a';
									break;
								case 'b':
									c = '\b';
									break;
								case 'f':
									c = '\f';
									break;
								case 'n':
									c = '\n';
									break;
								case 'r':
									c = '\r';
									break;
								case 't':
									c = '\t';
									break;
								case 'v':
									c = '\v';
									break;
								default: // All advanced cases
									if (c2 >= '0' and c2 <= '7' and it != str.end()) { // 8-based number
											char c3 = *it++;
											if (c3 >= '0' and c3 <= '7' and it != str.end()) {
													char c4 = *it++;
													if (c4 >= '0' and c4 <= '7') {
															const char a[4] = {c2, c3, c4, '\0'};
															c = strtol(a, NULL, 8);
															}
													else {
															fail();
															}
													}
											else if (c2 == '0') {
													c = '\0';
													}
											else {
													fail();
													}
											}
									else if (c2 == 'x' and it != str.end()) {
											auto checkHex = [](char c) {
												return ((c >= '0' and c <= '9') or (c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F'));
												};
											char c3 = *it++;
											if (checkHex(c3) and it != str.end()) {
													char c4 = *it++;
													if (checkHex(c4)) {
															char a[3] = {c3, c4, '\0'};
															c = strtol(a, NULL, 16);
															}
													else {
															fail();
															}
													}
											else {
													fail();
													}
											}
									else {
											fail();
											}
								}
						}
				res += c;
				}
		return res;
		}

	std::unique_ptr<sqlite::database> markovBackend::connect() {
		sqlite::sqlite_config conf;
		conf.flags = sqlite::OpenFlags::READWRITE | sqlite::OpenFlags::CREATE | sqlite::OpenFlags::NOMUTEX | sqlite::OpenFlags::SHAREDCACHE | sqlite::OpenFlags::URI;
		conf.encoding = sqlite::Encoding::UTF8; // Fuck UTF16, use UTF8!
		auto db = std::make_unique<sqlite::database>(database_uri, conf);
		*db << "PRAGMA synchronous = 0;"; // Our db isn't critical
		*db << "PRAGMA journal_mode = MEMORY;";
		auto busy = [](void *, int count) {
			unsigned long time = 1000;
			unsigned long tick = 1;
			if (count < time/tick) {
				std::this_thread::sleep_for(std::chrono::milliseconds(tick));
				return 1;
			} else {
				std::cout << "Терпение кончилось!" << std::endl;
				return 0;
			}
		};
		sqlite3_busy_handler(&*db->connection(), busy, nullptr);
		return db;
		}

	void markovBackend::init(std::vector<std::string> opts) { // TODO: add more foolproof
		if (opts.size() != 1) {
				throw std::invalid_argument("You should give only config file name to markov backend (via backend-opts) or helpme to display help");
				};
		po::options_description config("Config file");
		config.add_options()
		("iter", po::value<std::string>()->required(), "iterator (to select block, regex)")
		("prefixmiddle", po::value<std::string>()->required(), "insert between captured patterns")
		("N", po::value<unsigned int>()->required(), "consider N captures")
		("splitstr", po::value<bool>()->required(), "parse string by string, not all file")
		("maxgen", po::value<unsigned long long int>()->required(), "do not print more than maxgen block  (zero to no limit)")
		("rndstart", po::value<bool>()->default_value(false, "false"), "start from random combination")
		("separator", po::value<std::string>(), "block separator (regex)")
		("database_uri", po::value<std::string>()->default_value("\"file:memdb1?mode=memory\""), "valid URI to sqlite database, using `?cache=shared` is recommended")
		("sqlite_table", po::value<std::string>()->required(), "main SQLite3 table")
		("sqlite_table_dict", po::value<std::string>()->required(), "dictionary SQLite3 table")
		("sqlite_index", po::value<bool>()->default_value(false), "use index or not (it is recommended )");
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

		database_uri = configString("database_uri", vm);
		sqlite_table = configString("sqlite_table", vm);
		sqlite_table_dict = configString("sqlite_table_dict", vm);
		sqlite_index = vm["sqlite_index"].as<bool>();

			{
			std::vector<std::string> vec;
			std::vector<std::string> vec2 = {"rstr"};
			for (int i = 1; i <= N; i++) {
					vec.push_back("str"+std::to_string(i)+"=?"); // All "base" strings
					vec2.push_back("str"+std::to_string(i));
					}
			sqlite_query = "SELECT rstr FROM "+sqlite_table+" WHERE "+joinStr(vec, " AND ")+" ORDER BY RANDOM() LIMIT 1;";
			sqlite_insert = "INSERT INTO "+sqlite_table+" ("+joinStr(vec2, ", ")+") VALUES("+repeatDelim("?", ", ", N+1)+");";

			sqlite_insert_dict = "INSERT INTO "+sqlite_table_dict+" (str) VALUES(?);";
			if (sqlite_index) {
					dict_idx_name = sqlite_table_dict+"_index";
					table_idx_name = sqlite_table+"_index";
					}
			}

		mainConnection = connect();
		std::cout << "Connect OK!" << std::endl;
		};

	rowid_t markovBackend::dictID(const std::string& str, sqlite::database& db) {
		rowid_t id = 0;
		db << "SELECT rowid FROM "+sqlite_table_dict+" WHERE str=? LIMIT 1;"
		   << str
		   >> id;
		return id;
		};

	std::string markovBackend::dictStr(const markov::rowid_t& id, sqlite::database& db) {
		std::string str = "";
		db << "SELECT str FROM "+sqlite_table_dict+" WHERE rowid=? LIMIT 1;"
		   << id
		   >> str;
		return str;
		};

	void markovBackend::trainBegin(std::vector<std::shared_ptr<std::ifstream>> arr) {
		auto&& db = *mainConnection;
		db << "CREATE TABLE IF NOT EXISTS "+sqlite_table_dict+" (id INTEGER PRIMARY KEY ASC, str TEXT NOT NULL UNIQUE ON CONFLICT IGNORE);"; // id is only for human redactors via SqliteBrowser
		db << "INSERT INTO "+sqlite_table_dict+"(rowid, str) VALUES(?, ?);" << 0 << ""; // Special value
			{
			std::vector<std::string> vec = {"rstr INTEGER NOT NULL"};
			for (unsigned int i = 1; i <= N; ++i) {
					vec.push_back("str"+std::to_string(i)+" INTEGER NOT NULL"); // All "base" strings
					}
			db << "CREATE TABLE IF NOT EXISTS "+sqlite_table+" ("+joinStr(vec, ", ")+");";
			}
		if (sqlite_index) {
				db << "DROP INDEX IF EXISTS "+dict_idx_name+";";
				db << "DROP INDEX IF EXISTS "+table_idx_name+";";
				}
		std::cout << "Building dictionary… ";
		db << "begin;";
		auto pstm = db << sqlite_insert_dict;
		auto dictAdd = [&pstm](std::string str, bool) {
			pstm << str;
			pstm++;
			};
		for (auto& file: arr) {
				trainFile(file, dictAdd);
				};
		db << "commit;";
		std::cout << "done!" << std::endl;
		if (sqlite_index) {
				std::cout << "Building dictionary index… ";
				db << "CREATE INDEX "+dict_idx_name+" ON "+sqlite_table_dict+" (str);";
				std::cout << "done!" << std::endl;
				}
		};

	void markovBackend::trainFile(std::shared_ptr<std::ifstream> file, std::function<void(const std::string&, const bool&)> func) {
		std::string content;
			{
			file->seekg(0, std::ios::end);
			content.reserve(file->tellg());
			file->seekg(0, std::ios::beg);

			content.assign((std::istreambuf_iterator<char>(*file)),
						   std::istreambuf_iterator<char>());
			}
			{
			if (separator != boost::regex("")) {
					boost::sregex_token_iterator partsIter(content.begin(), content.end(), separator, -1);
					while (partsIter != xInvalidTokenIt) {
							trainPart(*partsIter++, func);
							}
					}
			else {
					trainPart(content, func);
					}
			}
		}

	void markovBackend::trainPart(const std::string& data, std::function<void(const std::string&, const bool&)> func) {
		if (splitstr) {
				boost::sregex_token_iterator linesIter(data.begin(), data.end(), boost::regex("\n+"), -1);
				while (linesIter != xInvalidTokenIt) {
						trainFinal(*linesIter++, func);
						func("\n", false);
						}
				}
		else {
				trainFinal(data, func);
				}
		func("", true); // Insert end*/
		};

	void markovBackend::trainFinal(const std::string& data, std::function<void(const std::string&, const bool&)> func) {
		boost::sregex_iterator blocksIter(data.begin(), data.end(), boost::regex(iter));
		while (blocksIter != xInvalidIt) {
				func((blocksIter++)->str(), false);
				}
		};

	void markovBackend::trainInsert(const std::string& data, sqlite::database db, sqlite::database_binder& pstm, MarkovDeque& dq) {
		rowid_t id = dictID(data, db);
		pstm << id;
		for (int i = 1; i <= N; i++) {
				pstm << dq.at(i-1);
				}
		pstm.execute();
		shiftDeque(dq, id);
		}

	boost::any markovBackend::train(std::shared_ptr<std::ifstream> file) {
		auto db = connect();
		*db << "begin;";
		auto pstm = *db << sqlite_insert;
		MarkovDeque dq(N, 0);
		auto func = [&db, &pstm, &dq, this](const std::string& str, bool reset) {
			trainInsert(str, *db, pstm, dq);
			if (reset) {
					dq = MarkovDeque(N, 0);
				}
			};
		trainFile(file, func);
		*db << "commit;";
		file->close();

		return true;
		};

	boost::any markovBackend::merge(std::vector<boost::any>&) {
		auto&& db = *mainConnection;
		if (sqlite_index) {
				std::cout << "Building data index (it can take some time)… ";
				std::vector<std::string> vec;
				for (unsigned i = 1; i <= N; i++) {
						vec.push_back("str"+std::to_string(i));
						}
				db << "CREATE INDEX "+table_idx_name+" ON "+sqlite_table+" ("+joinStr(vec, ", ")+");";
				std::cout << "done!" << std::endl;
				};
		return true;
		};

	void markovBackend::out(boost::any&, std::shared_ptr<std::ostream> o) {
		auto&& db = *mainConnection;
		auto pstm = db << sqlite_query;
		MarkovDeque dq;
		if (rndstart) {
				dq = MarkovDeque(N);
				for (auto &&row : db << "SELECT * FROM "+sqlite_table+" ORDER BY RANDOM() LIMIT 1;") {
						for (unsigned int i = 0; i < N; ++i) row >> dq[i];
						};
				}
		else {
				dq = MarkovDeque(N, 0);
				}
		rowid_t res = outGet(pstm, dq);
		unsigned long long int n = 0;
		while (res != 0) {
				std::string str = dictStr(res, db);
				(*o) << str;
				if (str != "\n") {
						(*o) << prefixmiddle;
						};
				if (maxgen > 0) {
						if (++n == maxgen) {
								return;
								}; // We reached limit
						}
				shiftDeque(dq, res);
				res = outGet(pstm, dq);
				}
		};

	rowid_t markovBackend::outGet(sqlite::database_binder& pstm, MarkovDeque& dq) {
		for (int i = 1; i <= N; i++) {
				pstm << dq.at(i-1);
				}
		rowid_t res = 0;
		pstm >> res;
		return res;
		};

	markovBackend backendInterface;
	}

BOOST_DLL_ALIAS(markov::backendInterface, backendInterface) // Export variable
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

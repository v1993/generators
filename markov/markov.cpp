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

	void checkFile(std::string fname) {
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
			("separator", po::value<std::string>(), "block separator (regex)");
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

		boost::random_device seed_gen;
		gen = boost::mt19937(seed_gen()); 
	};

	void markovBackend::trainPart(std::string data, Hashtable& tab) {
		MarkovDeque dq(N, "");
		if (splitstr) {
			boost::sregex_token_iterator linesIter(data.begin(), data.end(), boost::regex("\n+"), -1);
			while(linesIter != xInvalidTokenIt) {
				trainFinal(*linesIter++, tab, dq);
				trainInsert("\n", tab, dq);
			}
		} else {
			trainFinal(data, tab, dq);
		}
		tab.emplace(dq, ""); // Insert end
	};

	void markovBackend::trainFinal(std::string data, Hashtable& tab, MarkovDeque& dq) {
		boost::sregex_iterator blocksIter(data.begin(), data.end(), boost::regex(iter));
		while (blocksIter != xInvalidIt) {
			trainInsert((blocksIter++)->str(), tab, dq);
		}
	};

	void markovBackend::trainInsert(std::string data, Hashtable& tab, MarkovDeque& dq) {
		tab.emplace(dq, data);
		shiftDeque<std::string>(dq, data);
	}

	boost::any markovBackend::train(std::shared_ptr<std::ifstream> file) {
		Hashtable tab;
		std::string content;
		{
			file->seekg(0, std::ios::end);   
			content.reserve(file->tellg());
			file->seekg(0, std::ios::beg);

			content.assign((std::istreambuf_iterator<char>(*file)),
							std::istreambuf_iterator<char>());
			file->close();
		}
		if (separator != boost::regex("")) {
			boost::sregex_token_iterator partsIter(content.begin(), content.end(), separator, -1);
			while(partsIter != xInvalidTokenIt) {
				trainPart(*partsIter++, tab);
			}
		} else {
			trainPart(content, tab);
		}
		return tab;
	};

	boost::any markovBackend::merge(std::vector<boost::any>& vec) {
		if (vec.size() == 1) { return vec.front(); }
		auto first = boost::any_cast<Hashtable>(vec.front());
		for(auto i = ++vec.begin(); i != vec.end(); ++i) {
			first.merge(boost::any_cast<Hashtable>(*i));
		}
		return first;
	};

	void markovBackend::out(boost::any& Atab, std::shared_ptr<std::ostream> o) {
		auto tab = boost::any_cast<Hashtable>(Atab);
		MarkovDeque dq;
		if (rndstart) {
			int num = boost::random::uniform_int_distribution<>(0, tab.size()-1)(gen);
			dq = std::next(tab.begin(), num)->first;
		} else {
			dq = MarkovDeque(N, "");
		}
		std::string str = outGet(tab, dq);
		unsigned long long int n = 0;
		while(str != "") {
			(*o) << str;
			if (str != "\n") { (*o) << prefixmiddle; };
			if (maxgen > 0) {
				if (++n == maxgen) { return; }; // We reached limit
			}
			shiftDeque(dq, str);
			str = outGet(tab, dq);
		}
	};

	std::string markovBackend::outGet(Hashtable& tab, MarkovDeque& dq) {
		if (tab.count(dq) == 0) { return ""; } // Not found at all, hopeless
		else if (tab.count(dq) == 1) { return tab.find(dq)->second; } // If only one math, return it
		/* else if (try_valid) { // Probably, remove this
			std::vector<std::string> vec;
			auto range = tab.equal_range(dq);
			for_each (range.first, range.second,
			[&vec](Hashtable::value_type& x) {
				if (x.second != "") {
					vec.push_back(x.second);
				};
			});
			if (vec.size() == 0) { return ""; } // Only endings, hopeless
			int num = boost::random::uniform_int_distribution<>(0, vec.size()-1)(gen);
			return vec.at(num);
		} */ else {
			int num = boost::random::uniform_int_distribution<>(0, tab.count(dq)-1)(gen);
			return std::next(tab.equal_range(dq).first, num)->second;
		}
	};

	markovBackend backendInterface;
}

BOOST_DLL_ALIAS(markov::backendInterface, backendInterface) // Export variable

#include "mainDefs.hpp"
#include "Parseopts.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

void validate(boost::any& v, std::vector<std::string> const &values, cacheop*, int) {
	using namespace boost::program_options;
	// Make sure no previous assignment to 'v' was made.
	validators::check_first_occurrence(v);

	// Extract the first string from 'values'. If there is more than
	// one string, it's an error, and exception will be thrown.
	std::string const& s = validators::get_single_string(values);
	std::vector<std::string> allowed {"r", "w", "a", ""};
	std::vector<std::string>::iterator it = find(allowed.begin(), allowed.end(), s);

	if (it != allowed.end()) {
			v = boost::any(cacheop(s));
			}
	else {
			throw validation_error(validation_error::invalid_option_value);
			}
	}

static void checkFile(std::string fname) {
	if (!boost::filesystem::exists(fname)) {
			throw std::invalid_argument("file `"+fname+"` not found.");
			}
	}

struct opts parseOpts(int ac, char* av[]) {
	struct opts o;
	try {
			// Declare a group of options that will be
			// allowed only on command line
			po::options_description generic("Common");
			generic.add_options()
			("output,o", po::value< std::string >(), "output file")
			("no-end", "do not write '\\n' at the end")
			("backend,b", po::value<std::string>()->required(), "REQUIRED path to backend")
			("backend-opts,p", po::value<std::vector<std::string>>()->default_value(std::vector<std::string>(), "empty"), "parametrs to backend (can be used multiplie times)")
			("jobs,j", po::value<unsigned int>()->default_value(1), "maximal count of jobs at one time (1 per file) (1 by default)")
			("cache,c", po::value <cacheop>()->default_value(cacheop(""), "empty"), "cache operation (r=read, w=write, a=append, no option=do not use caching) requires --cache-file")
			("cache-file,f", po::value <std::string>()->default_value(""), "cache file to use (or another way to determine cache, like table name, optional)")
			("help,h", "print help message")
			("version,v", "print version string");
			po::options_description hidden("Hidden options");
			hidden.add_options()
			("input-files", po::value< std::vector<std::string>>(), "input files (stdin by default)");


			po::options_description cmdline_options;
			cmdline_options.add(generic).add(hidden);


			po::positional_options_description p;
			p.add("input-files", -1);

			po::variables_map vm;
			store(po::command_line_parser(ac, av).
				  options(cmdline_options).positional(p).run(), vm);

			if (vm.count("help")) {
					std::cout << "This is generators frontend part - applictation for generating new sequence based on your own. You need backend part to use." << std::endl;
					std::cout << "Usage: " << boost::filesystem::basename(av[0]) << " -b BACKEND [options] [INPUT_FILES]..." << std::endl;
					std::cout << generic << std::endl;
					exit(EXIT_SUCCESS);
					}

			if (vm.count("version")) {
					std::cout << "Generators, version 0.1 alpha" << std::endl;
					exit(EXIT_SUCCESS);
					}

			po::notify(vm);

			if (vm.count("output")) {
					std::string fname = vm["output"].as<std::string>();
					auto ptr = std::make_shared<std::ofstream>();
					ptr->exceptions(std::ofstream::failbit | std::ofstream::badbit);
					ptr->open(fname, std::ofstream::trunc);
					o.out = ptr;
					}
			else {
					o.out = std::shared_ptr<std::ostream>(&std::cout, [](void*) {});
					}
			o.no_end = vm.count("no-end") ? true : false;
			o.backend = vm["backend"].as<std::string>();
			checkFile(o.backend);
			o.backend_opts = vm["backend-opts"].as<std::vector <std::string> >();
			o.jobs = vm["jobs"].as<unsigned int>();
			cacheop cop = vm["cache"].as<cacheop>();
			o.cache = cop;
			o.cachefile = vm["cache-file"].as<std::string>();

			if (!vm.count("input-files") and cop.value != "r") { // Make fancy message
					throw std::invalid_argument("you must specify input files");
					}
			else if (vm.count("input-files") and cop.value == "r") {
					throw std::invalid_argument("you don't need to set input files if you read from cache");
					}
			else if (cop.value == "r") {
					}
			else {
					for (auto const& fname: vm["input-files"].as<std::vector<std::string> >()) {
							checkFile(fname);
							auto ptr = std::make_shared<std::ifstream>();
							ptr->exceptions(std::ifstream::failbit | std::ifstream::badbit);
							ptr->open(fname);
							o.inpfiles.push_back(ptr);
							o.inpfiles_names[ptr] = fname;
							}
					}
			return o;
			}
	catch (std::exception& e)
			{
			std::cerr << "Error while handling arguments: " << e.what() << "\n";
			exit(EXIT_FAILURE);
			}
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

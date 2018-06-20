#ifndef I_V_MAIN_DEFS
#define I_V_MAIN_DEFS
#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <exception>
#include <vector>
#include <string>
#include <memory>
//#include <filesystem>

#include <cstdlib>

#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include <boost/dll/import.hpp>

struct cacheop {
	explicit cacheop(std::string const& val):
		value(val)
		{ }
	operator std::string() const { return value; }
	std::string value;
	};

struct opts {
	opts(): cache("") {}
	std::shared_ptr<std::ostream> out;
	bool no_end = false;
	std::string backend;
	std::vector<std::string> backend_opts;
	unsigned int jobs;
	cacheop cache;
	std::string cachefile = "";
	std::vector<std::shared_ptr<std::ifstream>> inpfiles;
	std::map<std::shared_ptr<std::ifstream>, std::string> inpfiles_names;
	};

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

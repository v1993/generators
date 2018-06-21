#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

#include <boost/any.hpp>

class generatorAPI {
	public:
		generatorAPI() {};
		virtual ~generatorAPI() {};

		virtual void init(std::vector<std::string>) = 0; // Arguments: backend-options from cmd

		virtual void trainBegin(std::vector<std::shared_ptr<std::ifstream>>) = 0; // Arguments: vector of smart pointers to input files
		virtual boost::any train(std::shared_ptr<std::ifstream>) = 0; // Arguments: smart pointer to input file
		virtual boost::any merge(std::vector<boost::any>&) = 0; // Arguments: vector of results from `train`
		virtual void out(boost::any&, std::shared_ptr<std::ostream>) = 0; // Arguments: value from `merge` or `load`, smart pointer to output file

		virtual boost::any load(std::string) = 0; // Arguments: filename to load from
		virtual void save(std::string, boost::any&) = 0; // Arguments: filename to save to, data from `merge`
	};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

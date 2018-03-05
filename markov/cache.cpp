// This file implements save/load functions

#include "interface.hpp"

#include <iostream>
#include <fstream>

#include <boost/serialization/serialization.hpp>

#if MARKOV_OPT_MEMORY
#include <boost/serialization/map.hpp>
#else
#include <boost/serialization/unordered_map.hpp>
#endif

#include <boost/serialization/string.hpp>
#include <boost/serialization/deque.hpp>


#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace markov {
	void markovBackend::save(std::string fname, boost::any& data) {
		std::ofstream file;
		file.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
		file.open(fname, std::ofstream::trunc);
		{
			Hashtable tab = boost::any_cast<Hashtable>(data);
			boost::archive::text_oarchive oarch(file);
			oarch << tab;
		}
		file.close();
	};

	boost::any markovBackend::load(std::string fname) {
		checkFile(fname);
		std::ifstream file;
		file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
		file.open(fname);
		Hashtable tab;
		{
			boost::archive::text_iarchive iarch(file);
			iarch >> tab;
		}
		file.close();
		return tab;
	};
}

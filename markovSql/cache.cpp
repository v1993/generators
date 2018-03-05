// MySQL don't need save/load

#include "interface.hpp"

namespace markov {
	void markovBackend::save(std::string fname, boost::any& data) {
	};

	boost::any markovBackend::load(std::string fname) {
		return true;
	};
}

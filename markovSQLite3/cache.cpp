#include "interface.hpp"

namespace markov {
	void markovBackend::save(__attribute__((unused)) std::string fname, __attribute__((unused)) boost::any& data) {
		};

	boost::any markovBackend::load(std::string fname) {
		return true;
		};
	}
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

#define MARKOV_OPT_MEMORY false

#include <generatorAPI.hpp>

#if MARKOV_OPT_MEMORY
#include <map>
#else
#include <unordered_map>
#endif

#include <deque>
#include <utility>
#include <boost/regex.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/functional/hash.hpp>

template <typename Container> // we can make this generic for any container
struct container_hash {
    std::size_t operator()(Container const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};

namespace markov {
	using MarkovDeque = std::deque<std::string>;
	#if MARKOV_OPT_MEMORY
	using Hashtable = std::multimap<MarkovDeque, std::string>;
	#else
	using Hashtable = std::unordered_multimap<MarkovDeque, std::string, container_hash<MarkovDeque>>;
	#endif
	template<typename T>
	void shiftDeque(std::deque<T>&, T&);
	void checkFile(std::string fname);
	class markovBackend: public generatorAPI {
		public:
			void init(std::vector<std::string>);
			boost::any train(std::shared_ptr<std::ifstream>);
			boost::any merge(std::vector<boost::any>&);
			void out(boost::any&, std::shared_ptr<std::ostream>);

			boost::any load(std::string);
			void save(std::string, boost::any&);

		protected:
			void trainPart(std::string, Hashtable&);
			void trainFinal(std::string, Hashtable&, MarkovDeque&);
			void trainInsert(std::string, Hashtable&, MarkovDeque&);

			std::string outGet(Hashtable&, MarkovDeque&);

			std::string iter;
			std::string prefixmiddle;
			unsigned int N;
			bool splitstr;
			unsigned long long int maxgen;
			bool rndstart;
			boost::regex separator;

			boost::sregex_token_iterator xInvalidTokenIt;
			boost::sregex_iterator xInvalidIt;

			boost::mt19937 gen; 
	};
}

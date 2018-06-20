#include <generatorAPI.hpp>
#include <boost/dll/alias.hpp>

namespace testBackendNamespace {

	const std::string s = "Hey!";

	class testBackend: public generatorAPI {
		public:
			void init(std::vector<std::string>) {std::cout << "init() called" << std::endl;};
			void trainBegin(std::vector<std::shared_ptr<std::ifstream>>) {std::cout << "trainBegin() called" << std::endl;};
			boost::any train(std::shared_ptr<std::ifstream>) {std::cout << "train() called" << std::endl; return s;};
			boost::any merge(std::vector<boost::any>&) {std::cout << "merge() called" << std::endl; return s;};
			void out(boost::any&, std::shared_ptr<std::ostream>) {std::cout << "out() called" << std::endl;};

			boost::any load(std::string) {std::cout << "load() called" << std::endl; return s;};
			void save(std::string, boost::any&) {std::cout << "save() called" << std::endl;};
	};

	testBackend backendInterface;
}

BOOST_DLL_ALIAS(testBackendNamespace::backendInterface, backendInterface) // Export variable

#include <generatorAPI.hpp>
#include "mainDefs.hpp"
#include "Parseopts.hpp"

namespace dll = boost::dll;
boost::shared_ptr<generatorAPI> backend;

void trainFile(std::shared_ptr<std::ifstream> file, std::string fname, std::vector<boost::any> *vec, std::mutex *mutex) {
	std::cout << "Started parsing file " << fname << std::endl;
	boost::any result = backend->train(file);
	std::cout << "Finished parsing file " << fname << std::endl;
	mutex->lock();
	vec->push_back(result);
	mutex->unlock();
}

std::vector<boost::any> trainAll(opts o) {
	std::mutex mutex;
	std::vector<boost::any> vec;
	boost::asio::thread_pool pool(o.jobs);

	for(auto const& file: o.inpfiles) {
		boost::asio::post(pool, boost::bind(trainFile, file, o.inpfiles_names[file], &vec, &mutex));
	}
	pool.join(); // Wait for all jobs
	return vec;
}

int main(int ac, char* av[]) {
	try {
		opts o = parseOpts(ac, av);
		std::cout << "Parsing options finished" << std::endl;
		backend = dll::import_alias<generatorAPI>(o.backend, "backendInterface", dll::load_mode::default_mode);
		std::cout << "Backend loaded" << std::endl;
		backend->init(o.backend_opts);
		std::cout << "Backend successfully initialisated" << std::endl;

		boost::any backendData;

		bool loadcache = o.cache.value == "r" or (o.cache.value == "a" and boost::filesystem::exists(o.cachefile));
		if (loadcache) { // If file does not exist, append acts like write
			backendData = backend->load(o.cachefile);
			std::cout << "Cache loaded " << std::endl;
		}

		if (o.cache.value != "r") {
			auto trainRes = trainAll(o);
			if (loadcache) { // Add preloaded data
				trainRes.push_back(backendData);
			}
			backendData = backend->merge(trainRes);
			std::cout << "Training finished successfully" << std::endl;
		}

		if (o.cache.value == "w" or o.cache.value == "a") {
			backend->save(o.cachefile, backendData);
			std::cout << "Cache saved, exiting..." << std::endl;
			exit(EXIT_SUCCESS);
		}

		std::cout << "Ready to out, starting it..." << std::endl;
		backend->out(backendData, o.out);
		if (!o.no_end) { (*o.out) << std::endl; };
		std::cout << "Out finished, exiting..." << std::endl;
		exit(EXIT_SUCCESS);
	}
	catch(std::exception &e) {
		std::cerr << "Error in main code: " << e.what() << "\n";
		exit(EXIT_FAILURE);
	}
}

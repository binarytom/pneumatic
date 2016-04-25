#include <iostream>
#include <fstream>
#include <boost/asio/buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "detail.h"
#include "Log.h"

using namespace std;

class pmat_file {
public:
	pmat_file(
		const char *data,
		size_t len
	) {
		pmat::state_t pm;
		pmat::header fr;
		asio::const_buffer remainder;
		std::tie(fr, remainder) = detail::read<pmat::header>(asio::buffer(data, len), pm);
		DEBUG << "PMAT state now has " << pm.types.size() << " types - " << (void *)(&pm);
		DEBUG << "Magic (\"PMAT\"): " << fr.magic;
		DEBUG << "Flags:";
		DEBUG << " * Big-endian:  " << (fr.flags.big_endian ? "yes" : "no");
		DEBUG << " * Int64:       " << (fr.flags.integer_64 ? "yes" : "no");
		DEBUG << " * Ptr64:       " << (fr.flags.pointer_64 ? "yes" : "no");
		DEBUG << " * Long double: " << (fr.flags.float_64 ? "yes" : "no");
		DEBUG << " * Threads:     " << (fr.flags.threads ? "yes" : "no");
		header_flags_ = fr.flags;

		perl_version_ = net::ntoh(fr.perl_ver);
		pmat_version_ = (static_cast<uint16_t>(fr.major_ver) << 8) | static_cast<uint16_t>(fr.minor_ver);
		DEBUG << "PMAT format " << pmat_version_string() << " generated on Perl " << perl_version_string();

		DEBUG << "Roots:";
		pmat::roots roots;
		std::tie(roots, remainder) = detail::read<pmat::roots>(remainder, pm);
		DEBUG << "Stack:";
		pmat::stack stack;
		std::tie(stack, remainder) = detail::read<pmat::stack>(remainder, pm);
		DEBUG << "Heap:";
		pmat::heap heap;
		std::tie(heap, remainder) = detail::read<pmat::heap>(remainder, pm);

		pm.finish();
	}

	std::string perl_version_string() const {
		int rev = (perl_version_) & 0xFF;
		int ver = (uint16_t) ((perl_version_ >>  8) & 0xFFFF);
		int sub = (uint16_t) ((perl_version_ >> 24) & 0xFFFF);
		return to_string(rev) + "." + to_string(ver) + "." + to_string(sub);
	}

	std::string pmat_version_string() const {
		return to_string(pmat_version_ >> 8) + "." + to_string(pmat_version_ & 0xFF);
	}

private:
	uint32_t perl_version_;
	uint16_t pmat_version_;
	pmat::flags_t header_flags_;
};

int
main(int argc, char **argv) {
	namespace po = boost::program_options;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", u8"show program options")
		("trace", po::value<bool>(), u8"excessive debug tracing output")
	;

	po::variables_map vm;
	po::store(
		po::parse_command_line(
			argc, argv, desc
		),
		vm
	);
	po::notify(vm);    

	if(vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}

	if (vm.count("trace")) {
	} else {
		namespace logging = boost::log;
		logging::core::get()->set_filter(
			/* phoenix template thingey, comparison is stored not the result */
			logging::trivial::severity >= logging::trivial::severity_level::debug
		);
	}

	DEBUG << "Starting parser";

	std::string filename { "sample.pmat" };
	if(argc > 1) {
		DEBUG << " argc = " << (int)argc;
		for(int i = 0; i < argc; ++i) {
			auto s = std::string { argv[i] };
			DEBUG << i << " - " << s;
			filename = s;
		}
	}	
	size_t bytes;
	{ // Work out how much we need to map
		ifstream file(filename, ios::binary | ios::ate);
		bytes = file.tellg();
	}

	// Map entire file
	boost::iostreams::mapped_file_source file;
	file.open(filename, bytes);
	if(!file.is_open()) {
		ERROR << "Could not mmap() the file";
		exit(-1);
	}

	pmat_file pf { file.data(), bytes };
	DEBUG << "Done";
	return 0;
}

#include <iostream>
#include <fstream>
#include <boost/asio/buffer.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "detail.h"
#include "Log.h"

using namespace std;

int main(int argc, char **argv) {
	if(true) {
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

	const char *data = file.data();
	//while(is) {
		auto len = bytes;

		pmat::state_t pm;
		pmat::header fr;
		asio::const_buffer remainder;
		std::tie(fr, remainder) = detail::read<pmat::header>(asio::buffer(data, len), pm);
		DEBUG << "PMAT state now has " << pm.types.size() << " types - " << (void *)(&pm);
		DEBUG << "Magic: " << fr.magic;
		DEBUG << "Flags:";
		DEBUG << " * Big-endian:  " << (fr.flags.big_endian ? "yes" : "no");
		DEBUG << " * Int64:       " << (fr.flags.integer_64 ? "yes" : "no");
		DEBUG << " * Ptr64:       " << (fr.flags.pointer_64 ? "yes" : "no");
		DEBUG << " * Long double: " << (fr.flags.float_64 ? "yes" : "no");
		DEBUG << " * Threads:     " << (fr.flags.threads ? "yes" : "no");
		uint32_t pv = net::ntoh(fr.perl_ver);
		int rev = (pv      ) & 0xFF;
		int ver = (uint16_t) ((pv >>  8) & 0xFFFF);
		int sub = (uint16_t) ((pv >> 24) & 0xFFFF);
		DEBUG << "PMAT format " << to_string(fr.major_ver) << "." << to_string(fr.minor_ver) << " generated on Perl " << to_string(rev) << "." << to_string(ver) << "." << to_string(sub);

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
	//}
	DEBUG << "Done";
	return 0;
}

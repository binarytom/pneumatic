#include <iostream>
#include <fstream>
#include <boost/asio/buffer.hpp>

#include "detail.h"
#include "Log.h"

using namespace std;

int main(void) {
	DEBUG << "Starting parser";

	ifstream is;
	is.open("sample.pmat", ios::binary);
	char *data = new char[256 * 1024];
	while(is) {
		is.read(data, 256 * 1023);
		auto len = is.gcount();

		pmat::header fr;
		asio::const_buffer remainder;
		std::tie(fr, remainder) = detail::read<pmat::header>(asio::buffer(data, len));
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
	}
	DEBUG << "Done";
	return 0;
}

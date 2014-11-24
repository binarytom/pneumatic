#ifndef PMAT_H
#define PMAT_H

#include <boost/fusion/include/define_struct.hpp>

namespace pmat {
	/* PMAT magic */
	using header_magic_t = std::integral_constant<uint32_t, 0x504d4154>;
	using null_byte_t = std::integral_constant<uint8_t, 0x00>;

	struct flags_t {
		/** True if this is big-endian */
		bool big_endian;
		/** Integers (INT/UV/IV) are 64-bit */
		bool integer_64;
		/** Pointers are 64-bit */
		bool pointer_64;
		/** Floats (NV) are 64-bit */
		bool float_64;
		/** ithreads enabled */
		bool threads;
	};
};

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), header,
	(pmat::header_magic_t, magic)
	(pmat::flags_t, flags)
	(pmat::null_byte_t, reserved1)
	(uint8_t, major_ver)
	(uint8_t, minor_ver)
	(uint32_t, perl_ver)
)

#endif


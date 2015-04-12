#ifndef PMAT_H
#define PMAT_H

#include <vector>
#include <boost/fusion/include/define_struct.hpp>

namespace pmat {
	/* PMAT magic */
	using header_magic_t = std::integral_constant<uint32_t, 0x54414d50>;
	using null_byte_t = std::integral_constant<uint8_t, 0x00>;
	using ptr_t = void *;

	template<typename U, typename T>
	class vec : public std::vector<T> {
	public:
		vec():count{},items{} { }
	virtual ~vec() { }
		U count;
		std::vector<T> items;
	};

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

	typedef enum {
		SVt_NULL,       /* 0 */
		SVt_BIND,       /* 1 */
		SVt_IV,         /* 2 */
		SVt_NV,         /* 3 */
		/* RV was here, before it was merged with IV.  */
		SVt_PV,         /* 4 */
		SVt_PVIV,       /* 5 */
		SVt_PVNV,       /* 6 */
		SVt_PVMG,       /* 7 */
		SVt_REGEXP,     /* 8 */
		/* PVBM was here, before BIND replaced it.  */
		SVt_PVGV,       /* 9 */
		SVt_PVLV,       /* 10 */
		SVt_PVAV,       /* 11 */
		SVt_PVHV,       /* 12 */
		SVt_PVCV,       /* 13 */
		SVt_PVFM,       /* 14 */
		SVt_PVIO,       /* 15 */
		SVt_LAST        /* keep last in enum. used to size arrays */
	} svtype;
	
	enum class sv_type_t : std::uint8_t {
		SVtEND = 0,
		SVtGLOB, // 1
		SVtSCALAR, // 2
		SVtREF, // 3
		SVtARRAY, // 4
		SVtHASH, // 5
		SVtSTASH, // 6
		SVtCODE, // 7
		SVtMAGIC = 0x80,
		SVtUNKNOWN = 0xFF
	};
	enum class sv_code_type_t : std::uint8_t {
		SVCtEND = 0,
	  SVCtCONSTSV = 1,
	  SVCtCONSTIX,
	  SVCtGVSV,
	  SVCtGVIX,
	  SVCtPADNAME = 5,
	  SVCtPADSV = 6,
	  SVCtPADNAMES = 7,
	  SVCtPAD
	};

	enum class sv_ctx_type_t : std::uint8_t {
		PMAT_CTXtSUB = 1,
		PMAT_CTXtTRY,
		PMAT_CTXtEVAL,
	};

};

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), root,
	(std::string, rootname)
	(pmat::ptr_t, ptr)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), magic_t,
	(pmat::ptr_t, addr)
	(uint8_t, type)
	(uint8_t, flags)
	(pmat::ptr_t, obj)
	(pmat::ptr_t, ptr)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), type,
	(uint8_t, headerlen)
	(uint8_t, nptrs)
	(uint8_t, nstrs)
)

namespace pmat {
	using typevec8_t = pmat::vec<uint8_t, pmat::type>;
	using typevec16_t = pmat::vec<uint16_t, pmat::type>;
	using typevec32_t = pmat::vec<uint32_t, pmat::type>;
	using rootvec32_t = pmat::vec<uint32_t, pmat::root>;
	using ptrvec64_t = pmat::vec<uint64_t, pmat::ptr_t>;
	using bytevec32_t = pmat::vec<uint32_t, uint8_t>;

};

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), roots,
	(pmat::ptr_t, undef)
	(pmat::ptr_t, yes)
	(pmat::ptr_t, no)
	(pmat::rootvec32_t, other_roots)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), stack,
	(pmat::ptrvec64_t, elem)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv,
	(pmat::sv_type_t, type)
	(pmat::ptr_t, address)
	(uint32_t, refcnt)
	(uint64_t, size)
	(pmat::ptr_t, blessed)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_scalar,
	(uint8_t, flags)
	(uint64_t, iv)
	(double, nv)
	(uint64_t, pvlen)
	(pmat::ptr_t, ourstash)
	(std::string, pv)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_constsv,
	(pmat::ptr_t, target_sv)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_gvsv,
	(pmat::ptr_t, target_sv)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_constix,
	(uint64_t, padix)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_gvix,
	(uint64_t, padix)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_padnames,
	(pmat::ptr_t, names)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_pad,
	(uint64_t, depth)
	(pmat::ptr_t, pad)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code,
	(uint64_t, line)
	(uint8_t, flags)
	(pmat::ptr_t, op_root)
	(pmat::ptr_t, stash)
	(pmat::ptr_t, glob)
	(pmat::ptr_t, outside)
	(pmat::ptr_t, padlist)
	(pmat::ptr_t, constval)
	(std::string, file)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_glob,
	(uint64_t, line)
	(pmat::ptr_t, stash)
	(pmat::ptr_t, scalar)
	(pmat::ptr_t, array)
	(pmat::ptr_t, hash)
	(pmat::ptr_t, code)
	(pmat::ptr_t, egv)
	(pmat::ptr_t, io)
	(pmat::ptr_t, form)
	(std::string, name)
	(std::string, file)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_array,
	(uint64_t, count)
	(uint8_t, flags)
	(std::vector<pmat::ptr_t>, elements)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), heap,
	(std::vector<pmat::sv>, svs)
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), header,
	(pmat::header_magic_t, magic)
	(pmat::flags_t, flags)
	(pmat::null_byte_t, reserved1)
	(uint8_t, major_ver)
	(uint8_t, minor_ver)
	(uint32_t, perl_ver)
	(pmat::typevec8_t, types)
	(pmat::roots, roots)
	(pmat::stack, stack)
	(pmat::heap, heap)
)

namespace pmat {
	class state_t {
	public:
		state_t() { }
		~state_t() { }
		std::vector<pmat::type> types;
	};
};


#endif


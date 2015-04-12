#ifndef PMAT_H
#define PMAT_H

#include <vector>
#include <map>
#include <boost/fusion/include/define_struct.hpp>

#include "BitField.h"
#include "Log.h"

namespace pmat {
	/* PMAT magic */
	using header_magic_t = std::integral_constant<uint32_t, 0x54414d50>;
	using null_byte_t = std::integral_constant<uint8_t, 0x00>;
	using ptr_t = void *;

	using uint = std::uint64_t;

	template<typename U, typename T>
	class vec : public std::vector<T> {
	public:
		vec():count{},items{} { }
	virtual ~vec() { }
		U count;
		std::vector<T> items;
	};

	using hash_elem_t = std::map<std::string, pmat::ptr_t>;

	union flags_t {
		uint8_t data;

		/** True if this is big-endian */
		BitField<0, 1> big_endian;
		/** Integers (INT/UV/IV) are 64-bit */
		BitField<1, 1> integer_64;
		/** Pointers are 64-bit */
		BitField<2, 1> pointer_64;
		/** Floats (NV) are 64-bit */
		BitField<3, 1> float_64;
		/** ithreads enabled */
		BitField<4, 1> threads;
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
		SVtIO, // 8
		SVtLVALUE, // 9
		SVtREGEXP, // 10
		SVtFORMAT, // 11
		SVtINVLIST, // 12
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
	using ptrvec_t = pmat::vec<::pmat::uint, pmat::ptr_t>;
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
	(pmat::ptrvec_t, elem)
)

namespace pmat {
	class sv {
	public:
		pmat::sv_type_t type;
		pmat::ptr_t address;
		uint32_t refcnt;
		uint64_t size;
		pmat::ptr_t blessed;

		sv():type{},address{},refcnt{0},size{0},blessed{} { }
		sv(const sv &v):type{v.type},address{v.address},refcnt{v.refcnt},size{v.size},blessed{v.blessed} { }
	};

	class sv_scalar:public sv {
	public:
		uint8_t flags;
		uint64_t iv;
		double nv;
		uint64_t pvlen;
		pmat::ptr_t ourstash;
		std::string pv;

		sv_scalar() { }
		sv_scalar(const sv &v):sv{v} { }
	};

	class sv_hash:public sv {
	public:
		pmat::uint count;
		pmat::ptr_t backrefs;
		pmat::hash_elem_t elements;
		sv_hash() { }
		sv_hash(const sv &v):sv{v} { }
	};
	class sv_stash:public sv_hash {
	public:
		pmat::ptr_t mro_linear_all;
		pmat::ptr_t mro_linear_current;
		pmat::ptr_t mro_nextmethod;
		pmat::ptr_t mro_isa;
		std::string name;
		sv_stash() { }
		sv_stash(const sv &v):sv_hash{v} { }
	};

	class sv_ref:public sv {
	public:
		uint8_t flags;
		pmat::ptr_t rv;
		pmat::ptr_t ourstash;
		sv_ref() { }
		sv_ref(const sv &v):sv{v} { }
	};

	class sv_glob:public sv {
	public:
		uint64_t line;
		pmat::ptr_t stash;
		pmat::ptr_t scalar;
		pmat::ptr_t array;
		pmat::ptr_t hash;
		pmat::ptr_t code;
		pmat::ptr_t egv;
		pmat::ptr_t io;
		pmat::ptr_t form;
		std::string name;
		std::string file;
		sv_glob() { }
		sv_glob(const sv &v):sv{v} { }
	};
	class sv_array:public sv {
	public:
		uint64_t count;
		uint8_t flags;
		std::vector<pmat::ptr_t> elements;
		sv_array() { }
		sv_array(const sv &v):sv{v} { }
	};

	class sv_io:public sv {
	public:
		pmat::ptr_t top;
		pmat::ptr_t format;
		pmat::ptr_t bottom;
		sv_io() { }
		sv_io(const sv &v):sv{v} { }
	};

	class sv_lvalue:public sv {
	public:
		uint8_t type;
		pmat::uint offset;
		pmat::uint length;
		pmat::ptr_t target;
		sv_lvalue() { }
		sv_lvalue(const sv &v):sv{v} { }
	};
};

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv,
	(pmat::sv_type_t, type)
	(pmat::ptr_t, address)
	(uint32_t, refcnt)
	(uint64_t, size)
	(pmat::ptr_t, blessed)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_scalar,
	(uint8_t, flags)
	(uint64_t, iv)
	(double, nv)
	(uint64_t, pvlen)
	(pmat::ptr_t, ourstash)
	(std::string, pv)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_hash,
	(pmat::uint, count)
	(pmat::ptr_t, backrefs)
	(pmat::hash_elem_t, elements)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_stash,
	(pmat::uint, count)
	(pmat::ptr_t, backrefs)
	(pmat::ptr_t, mro_linear_all)
	(pmat::ptr_t, mro_linear_current)
	(pmat::ptr_t, mro_nextmethod)
	(pmat::ptr_t, mro_isa)
	(std::string, name)
	(pmat::hash_elem_t, elements)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_ref,
	(uint8_t, flags)
	(pmat::ptr_t, rv)
	(pmat::ptr_t, ourstash)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_glob,
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

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_array,
	(uint64_t, count)
	(uint8_t, flags)
	(std::vector<pmat::ptr_t>, elements)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_io,
	(pmat::ptr_t, top)
	(pmat::ptr_t, format)
	(pmat::ptr_t, bottom)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_lvalue,
	(uint8_t, type)
	(pmat::uint, offset)
	(pmat::uint, length)
	(pmat::ptr_t, target)
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

	/*
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_invlist,
)
*/

	/*
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_re,
)

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_format,
)
*/

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
)

namespace pmat {
	class state_t {
	public:
		std::vector<pmat::type> types;
		std::map<pmat::ptr_t, pmat::sv*> sv_by_addr_;
		std::map<pmat::sv_type_t, size_t> sv_count_by_type_;
		std::map<pmat::sv_type_t, size_t> sv_size_by_type_;

		explicit state_t() { }
		~state_t() {
			DEBUG << "We ended up with the following counts:";
			for(auto &k : sv_count_by_type_) {
				DEBUG << " * " << sv_type_by_id(k.first) << " - " << (int) k.second;
			}
			DEBUG << "... and sizes:";
			size_t total = 0;
			for(auto &k : sv_size_by_type_) {
				DEBUG << " * " << sv_type_by_id(k.first) << " - " << (int) k.second;
				total += k.second;
			}
			DEBUG << "In total, there were " << (int)sv_by_addr_.size() << " SVs totalling " << (int)total << " bytes";
	   	}

		pmat::sv &sv_by_addr(const pmat::ptr_t &addr) const { return *(sv_by_addr_.at(addr)); }
	
		void add_sv(pmat::sv &sv) {
			sv_by_addr_[sv.address] = &sv;
			++sv_count_by_type_[sv.type];
			sv_size_by_type_[sv.type] += sv.size;
		}
	
		std::string sv_type_by_id(const sv_type_t &id) const {
			switch(id) {
			case sv_type_t::SVtEND: return "SVtEND";
			case sv_type_t::SVtGLOB: return "SVtGLOB";
			case sv_type_t::SVtSCALAR: return "SVtSCALAR";
			case sv_type_t::SVtREF: return "SVtREF";
			case sv_type_t::SVtARRAY: return "SVtARRAY";
			case sv_type_t::SVtHASH: return "SVtHASH";
			case sv_type_t::SVtSTASH: return "SVtSTASH";
			case sv_type_t::SVtCODE: return "SVtCODE";
			case sv_type_t::SVtIO: return "SVtIO";
			case sv_type_t::SVtLVALUE: return "SVtLVALUE";
			case sv_type_t::SVtREGEXP: return "SVtREGEXP";
			case sv_type_t::SVtFORMAT: return "SVtFORMAT";
			case sv_type_t::SVtINVLIST: return "SVtINVLIST";
			case sv_type_t::SVtMAGIC: return "SVtMAGIC";
			case sv_type_t::SVtUNKNOWN: return "SVtUNKNOWN";
			default: return "unknown sv type";
			}
		}
	};
};


#endif


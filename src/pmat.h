#pragma once

#include <vector>
#include <map>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <iomanip>

#include "BitField.h"
#include "Log.h"

namespace pmat {
	/* PMAT magic */
	using header_magic_t = std::integral_constant<uint32_t, 0x54414d50>;
	using null_byte_t = std::integral_constant<uint8_t, 0x00>;

	/* Actual size of int depends on perl build, but typically uint32/uint64 */
	using uint_t = std::uint64_t;
	/* Pointer also varies - we just assume 32/64 bit */
	using ptr_t = std::uint64_t;
	/* A hash provides string => value lookup, stick with pointers for now */
	using hash_elem_t = std::map<std::string, pmat::ptr_t>;


	template<typename U, typename T>
	class vec : public std::vector<T> {
	public:
		vec():count{},items{} { }
	virtual ~vec() { }
		U count;
		std::vector<T> items;
	};

	/* PMAT top-level flags - these mostly identify details about the build */
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

	/** Information about internal SV types, we don't use this anywhere yet */
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

	/**
	 * SV types - real and synthetic.
	 * Some of these are not found in the .pmat source, but applied post-facto
	 * during fixup.
	 */
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
		/* Start of synthetic types */
		SVtPADNAMES,
		SVtPADLIST,
		SVtPAD,
		/* End of synthetic types */
		SVtMAGIC = 0x80,
		SVtUNKNOWN = 0xFF
	};
	using ctx_type_t = uint8_t;

	/** The information attached to coderef bodies */
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

	/** Context body types */
	enum class sv_ctx_type_t : std::uint8_t {
		PMAT_CTXtEND = 0,
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

BOOST_FUSION_DEFINE_STRUCT(
	(pmat), context,
	(uint8_t, headerlen)
	(uint8_t, nptrs)
	(uint8_t, nstrs)
)

namespace pmat {
	using typevec8_t = pmat::vec<uint8_t, pmat::type>;
	using typevec16_t = pmat::vec<uint16_t, pmat::type>;
	using typevec32_t = pmat::vec<uint32_t, pmat::type>;
	using rootvec32_t = pmat::vec<uint32_t, pmat::root>;
	using ptrvec_t = pmat::vec<::pmat::uint_t, pmat::ptr_t>;
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
	std::string to_string( const pmat::ptr_t &ptr);
	std::string to_string( const pmat::sv &sv);

	class sv_invlist:public sv {
	public:
		sv_invlist() { }
		sv_invlist(const sv &v):sv{v} { }
	};
	class sv_scalar:public sv {
	public:
		uint8_t flags;
		uint64_t iv;
		double nv;
		pmat::uint_t pvlen;
		pmat::ptr_t ourstash;
		std::string pv;

		sv_scalar() { }
		sv_scalar(const sv &v):sv{v} { }
	};
	class sv_undef:public sv_scalar {
	public:
		sv_undef(pmat::ptr_t ptr) { address = ptr; type = pmat::sv_type_t::SVtSCALAR; iv = 0; nv = 0.0f; pvlen = 0; }
		sv_undef(const sv_scalar &v):sv_scalar{v} { }
	};
	class sv_yes:public sv_scalar {
	public:
		sv_yes(pmat::ptr_t ptr) { address = ptr; type = pmat::sv_type_t::SVtSCALAR; iv = 1; nv = 1.0f; pvlen = 3; pv = "yes"; }
		sv_yes(const sv_scalar &v):sv_scalar{v} { }
	};
	class sv_no:public sv_scalar {
	public:
		sv_no(pmat::ptr_t ptr) { address = ptr; type = pmat::sv_type_t::SVtSCALAR; iv = 0; nv = 0.0f; pvlen = 2; pv = "no"; }
		sv_no(const sv_scalar &v):sv_scalar{v} { }
	};

	class sv_hash:public sv {
	public:
		pmat::uint_t count;
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
		pmat::uint_t line;
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
		pmat::uint_t count;
		uint8_t flags;
		std::vector<pmat::ptr_t> elements;
		sv_array() { }
		sv_array(const sv &v):sv{v} { }
		sv_array(const sv_array &v):sv{v},count{v.count},flags{v.flags},elements{v.elements} { }
	};

	class sv_io:public sv {
	public:
        uint_t ifileno_;
        uint_t ofileno_;

		pmat::ptr_t top;
		pmat::ptr_t format;
		pmat::ptr_t bottom;
		sv_io() { }
		sv_io(const sv &v):sv{v} { }
	};

	class sv_lvalue:public sv {
	public:
		uint8_t type;
		pmat::uint_t offset;
		pmat::uint_t length;
		pmat::ptr_t target;
		sv_lvalue() { }
		sv_lvalue(const sv &v):sv{v} { }
	};

	class sv_regexp:public sv {
	public:
		sv_regexp() { }
		sv_regexp(const sv &v):sv{v} { }
	};

	class sv_format:public sv {
	public:
		sv_format() { }
		sv_format(const sv &v):sv{v} { }
	};

	class ctx {
	public:
		pmat::ctx_type_t type;
		uint8_t gimme;
		pmat::uint_t line;
		std::string file;

		ctx():type{},gimme{},line{0} { }
		ctx(
			const ctx &v
		):type(v.type),
		  gimme(v.type),
		  line(v.line),
		  file(v.file)
		{ }
	};
	std::string to_string( const pmat::ctx &ctx);

	class ctx_sub:public ctx {
	public:
		uint32_t old_depth;
		pmat::ptr_t cv;
		pmat::ptr_t args;

		ctx_sub() { }
		ctx_sub(const ctx &v):ctx{v} { }
	};
	class ctx_try:public ctx {
	public:
		ctx_try() { }
		ctx_try(const ctx &v):ctx{v} { }
	};
	class ctx_eval:public ctx {
	public:
		pmat::ptr_t cv;

		ctx_eval() { }
		ctx_eval(const ctx &v):ctx{v} { }
	};
};

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
	(pmat::uint_t, padix)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_gvix,
	(pmat::uint_t, padix)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_padnames,
	(pmat::ptr_t, names)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_pad,
	(pmat::uint_t, depth)
	(pmat::ptr_t, pad)
)
BOOST_FUSION_DEFINE_STRUCT(
	(pmat), sv_code_padname,
	(pmat::uint_t, padix)
	(std::string, padname)
	(pmat::ptr_t, ourstash)
)

namespace pmat {
	class sv_code:public sv {
	public:
		pmat::uint_t line;
		uint8_t flags;
		pmat::ptr_t op_root;
		uint32_t depth;
		pmat::ptr_t stash;
		pmat::ptr_t glob;
		pmat::ptr_t outside;
		pmat::ptr_t padlist;
		pmat::ptr_t constval;
		std::string file;
		std::string name;

		pmat::ptr_t constsv_;
		pmat::uint_t constix_;
		pmat::ptr_t gvsv_;
		pmat::uint_t gvix_;
		pmat::ptr_t padnames_;
		std::vector<pmat::ptr_t> pads_;

		std::vector<std::weak_ptr<pmat::sv>> pad_svs_;

		sv_code() { }
		sv_code(const sv &v):sv{v} { }
	};
	class sv_padlist:public sv_array {
	public:
		void set_cv(std::shared_ptr<pmat::sv_code> cv) { }
		sv_padlist() { }
		sv_padlist(const sv &v) = delete;
		sv_padlist(const sv_array &v):sv_array{v} { type = sv_type_t::SVtPADLIST; }
	};
	class sv_padnames:public sv_array {
	public:
		void set_cv(std::shared_ptr<pmat::sv_code> cv) { }
		sv_padnames() { }
		sv_padnames(const sv &v) = delete;
		sv_padnames(const sv_array &v):sv_array{v} { type = sv_type_t::SVtPADNAMES;  }
	};
	class sv_pad:public sv_array {
	public:
		void set_cv(std::shared_ptr<pmat::sv_code> cv) { }
		sv_pad() { }
		sv_pad(const sv &v) = delete;
		sv_pad(const sv_array &v):sv_array{v} { type = sv_type_t::SVtPAD;  }
	};
};

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv,
	(pmat::sv_type_t, type)
	(pmat::ptr_t, address)
	(uint32_t, refcnt)
	(pmat::uint_t, size)
	(pmat::ptr_t, blessed)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_scalar,
	(uint8_t, flags)
	(pmat::uint_t, iv)
	(double, nv)
	(pmat::uint_t, pvlen)
	(pmat::ptr_t, ourstash)
	(std::string, pv)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_hash,
	(pmat::uint_t, count)
	(pmat::ptr_t, backrefs)
	(pmat::hash_elem_t, elements)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_stash,
	(pmat::uint_t, count)
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
	(pmat::uint_t, line)
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
	(pmat::uint_t, count)
	(uint8_t, flags)
	(std::vector<pmat::ptr_t>, elements)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_io,
	(pmat::uint_t, ifileno_)
	(pmat::uint_t, ofileno_)
	(pmat::ptr_t, top)
	(pmat::ptr_t, format)
	(pmat::ptr_t, bottom)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_lvalue,
	(uint8_t, type)
	(pmat::uint_t, offset)
	(pmat::uint_t, length)
	(pmat::ptr_t, target)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::sv_code,
	(pmat::uint_t, line)
	(uint8_t, flags)
	(pmat::ptr_t, op_root)
	(pmat::ptr_t, stash)
	(pmat::ptr_t, glob)
	(pmat::ptr_t, outside)
	(pmat::ptr_t, padlist)
	(pmat::ptr_t, constval)
	(std::string, file)
)

BOOST_FUSION_ADAPT_STRUCT(
	pmat::ctx,
	(pmat::ctx_type_t, type)
	(uint8_t, gimme)
	(pmat::uint_t, line)
	(std::string, file)
)
BOOST_FUSION_ADAPT_STRUCT(
	pmat::ctx_sub,
	(uint32_t, old_depth)
	(pmat::ptr_t, cv)
	(pmat::ptr_t, args)
)
BOOST_FUSION_ADAPT_STRUCT(
	pmat::ctx_eval,
	(pmat::ptr_t, cv)
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
	(pmat), ctx_section,
	(std::vector<pmat::ctx>, contexts)
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
	(pmat::typevec8_t, contexts)
)

namespace pmat {
	class state_t {
	public:
		std::vector<pmat::type> types;
		std::vector<pmat::context> contexts;

		explicit state_t():file_offset_{0} { }
		~state_t() {
			dump_sizes();
	   	}

		size_t add_file_offset(const size_t v) { file_offset_ += v; }
		size_t file_offset() const { DEBUG << "File offset = " << file_offset_; return file_offset_; }

		/** Provides something like the pmat-sizes output */
		void dump_sizes() {
			/* First, combine blessed+regular SVs */
			struct Thing {
				size_t count, size;
			};
			auto types = std::map<std::string, Thing> { };
			for(auto &it : sv_count_by_blessed_type_) {
				types[it.first].count += it.second;
			}
			for(auto &it : sv_count_by_type_) {
				types[sv_type_by_id(it.first)].count += it.second;
			}
			for(auto &it : sv_size_by_blessed_type_) {
				types[it.first].size += it.second;
			}
			for(auto &it : sv_size_by_type_) {
				types[sv_type_by_id(it.first)].size += it.second;
			}

			using thing_type_t = std::pair<std::string, Thing>;
			auto myvec = std::vector<thing_type_t> { };
			for(auto it : types) {
				myvec.push_back(it);
			}
			size_t count = 0, total = 0;
			for(auto &it : myvec) {
				count += it.second.count;
				total += it.second.size;
			}

			/* Sort by size */
			std::sort(
				myvec.begin(),
				myvec.end(),
				[] (const std::pair<std::string, Thing> &left, const std::pair<std::string, Thing> &right) -> bool {
					return left.second.size > right.second.size;
				}
			);
			myvec.emplace_back(
				std::pair<std::string, Thing> {
					"Total",
					Thing {
						count,
						total
					}
				}
			);

			/* Now find the column widths */
			size_t width[3] { 0, 0, 0 };
			auto nmax = [](const size_t x, const size_t y) -> size_t { return x > y ? x : y; };
			for(const auto &it : myvec) {
				DEBUG << "Type " << it.first << " length is " << std::to_string(it.first.size());
				width[0] = nmax(width[0], it.first.size());
				width[1] = nmax(width[1], std::to_string(it.second.count).size());
				width[2] = nmax(width[2], std::to_string(it.second.size).size());
			}
			DEBUG << "width 0 " << width[0];
			DEBUG << "width 1 " << width[1];
			DEBUG << "width 2 " << width[2];
			{
				boost::io::ios_all_saver ias { std::cout };
				std::cout << std::setw(width[0]) << std::setiosflags(std::ios::left) << std::setfill(' ') << "Type";
				std::cout << std::setw(0) << " | ";
				std::cout << std::setw(width[1]) << std::setiosflags(std::ios::left) << std::setfill(' ') << "SVs";
				std::cout << std::setw(0) << " | ";
				std::cout << std::setw(width[2]) << std::setiosflags(std::ios::left) << std::setfill(' ') << "Bytes";
				std::cout << std::endl;
			}

			for(auto &it : myvec) {
				{
					boost::io::ios_all_saver ias { std::cout };
					std::cout << std::setw(width[0]) << std::setiosflags(std::ios::left) << std::setfill(' ') << it.first;
				}
				{
					boost::io::ios_all_saver ias { std::cout };
					std::cout << std::setw(0) << " | ";
					std::cout << std::setw(width[1]) << std::setfill(' ') << it.second.count;
					std::cout << std::setw(0) << " | ";
					std::cout << std::setw(width[2]) << std::setfill(' ') << it.second.size;
					std::cout << std::endl;
				}
			}
			DEBUG << "Expecting " << (int)sv_by_addr_.size() << " SVs, had " << count;
		}

		std::shared_ptr<pmat::sv> sv_by_addr(const pmat::ptr_t &addr) const { return sv_by_addr_.at(addr); }
	
		void dump_sv(const pmat::sv &sv) {
			DEBUG << pmat::to_string(sv.address) << ", type = " << (int) sv.type << " (" << sv_type_by_id(sv.type) << ")";
			switch(sv.type) {
			case pmat::sv_type_t::SVtSCALAR:
				{
					DEBUG << "Scalar";
				}
				break;
			case pmat::sv_type_t::SVtSTASH:
				{
					auto v = reinterpret_cast<const pmat::sv_stash *>(&sv);
					DEBUG << "Stash " << v->name;
				}
				break;
			default:
				DEBUG << "Unknown SV";
			}
		}

		/** Add an SV to our lists */
		void add_sv(std::shared_ptr<pmat::sv> sv) {
			DEBUG << "Adding SV " << pmat::to_string(*sv);
			assert(sv->type != sv_type_t::SVtEND);
			assert(sv->type != sv_type_t::SVtUNKNOWN);
			if(sv_by_addr_.cend() != sv_by_addr_.find(sv->address)) {
				auto existing = sv_by_addr_[sv->address];
				ERROR << "Already have address " << pmat::to_string(sv->address) << " occupied by " << sv_type_by_id(existing->type);
				dump_sv(*sv);
				dump_sv(*existing);
				// assert(sv_by_addr_.cend() == sv_by_addr_.find(sv->address));
				return;
			}
			sv_by_addr_[sv->address] = sv;
			if(sv->blessed == 0) {
				++sv_count_by_type_[sv->type];
				sv_size_by_type_[sv->type] += sv->size;
			} else {
				update_blessed(*sv);
			}
			DEBUG << "SV stats for [" << sv_type_by_id(sv->type) << "] on " << (void *)this << ": count " << sv_count_by_type_[sv->type] << " size " << sv_size_by_type_[sv->type];

			/* Was anything waiting for us to provide information for a blessed slot? */
			auto it = sv_blessed_pending_.find(sv->address);
			if(sv_blessed_pending_.cend() != it) {
				DEBUG << "Found " << it->second.size() << " items relying on this SV for blessed pointer";
				/* If something is using us as a blessed pointer, we must be a stash */
				assert(sv->type == sv_type_t::SVtSTASH);
				auto st = std::static_pointer_cast<pmat::sv_stash>(sv);
				DEBUG << "Type " << sv_type_by_id(st->type) << ", name is " << st->name;
				for(auto ptr : it->second) {
					auto v = sv_by_addr(ptr);
					assert(v->blessed == sv->address);
					DEBUG << pmat::to_string(v->address) << " from " << pmat::to_string(ptr) << " being updated - blessed was " << pmat::to_string(v->blessed) << " and we are " << pmat::to_string(sv->address);
					update_blessed(*v);
				}
				sv_blessed_pending_.erase(it);
			}

		}

		bool have_sv_at(pmat::ptr_t &ptr) const {
			return sv_by_addr_.cend() != sv_by_addr_.find(ptr);
	   	}

		std::shared_ptr<pmat::sv> sv_at(const pmat::ptr_t ptr) {
			return sv_by_addr_.at(ptr);
	   	}

		void update_blessed(pmat::sv &sv) {
			if(have_sv_at(sv.blessed)) {
				auto bt = sv_blessed_type(sv);
				DEBUG << "Entry from " << bt;
				++sv_count_by_blessed_type_[bt];
				sv_size_by_blessed_type_[bt] += sv.size;
			} else {
				DEBUG << "Could not find pointer for blessed stash " << (void *)sv.blessed << ", deferring " << (void*)sv.address;
				auto &t = sv_blessed_pending_[sv.blessed];
				t.push_back(sv.address);
				DEBUG << "Pending for " << (void *)sv.blessed << " is now " << t.size();
			}
		}

		void finish() {
			if(!sv_blessed_pending_.empty()) {
				ERROR << "Still had SVs that didn't resolve blessed pointer";
				sv_blessed_pending_.clear();
			}

			/* Apply fixup to every SV */
			for(auto it : sv_by_addr_) {
				auto sv = it.second;
				if(sv->type == pmat::sv_type_t::SVtCODE) {
					auto cv = std::static_pointer_cast<pmat::sv_code>(sv);
					DEBUG << "Have CODE SV at " << (void *)cv->address << " - " << cv->file << ":" << (int)cv->line;
					if(cv->padlist == 0) {
						INFO << "No PADLIST, skipping";
						continue;
					}

					if(!have_sv_at(cv->padlist)) {
						ERROR << "Padlist points to CV that does not exist - " << (void *)cv->padlist;
						continue;
					} else {
						DEBUG << "Upgrading padlist at address [" << (void *)cv->padlist << "]";
						auto old = std::static_pointer_cast<pmat::sv_array>(sv_at(cv->padlist));
						assert(old->type == sv_type_t::SVtARRAY);
						auto padlist = std::make_shared<pmat::sv_padlist>(*old);
						padlist->set_cv(cv);
						replace_sv(old, padlist);
					}
					assert(cv->padnames_ != 0);
					if(!have_sv_at(cv->padnames_)) {
						ERROR << "No SV for padnames at " << (void *)cv->padnames_;
					} else {
						DEBUG << "Upgrading padnames at address [" << (void *)cv->padnames_ << "]";
						auto old = std::static_pointer_cast<pmat::sv_array>(sv_at(cv->padnames_));
						assert(old->type == sv_type_t::SVtARRAY);
						auto padnames = std::make_shared<pmat::sv_padnames>(*old);
						assert(old->count == padnames->count);
						assert(old->elements.size() == padnames->elements.size());
						assert(old->elements.size() == padnames->count);
						padnames->set_cv(cv);
						replace_sv(old, padnames);

						DEBUG << "Total of " << cv->pads_.size() << " items to convert to pads";
						int idx = 0;
						for(auto &ptr : cv->pads_) {
							DEBUG << "Pad depth " << idx << " at " << (void *)ptr;
							if(idx > 0 && ptr != 0) {
								auto old_sv = sv_at(ptr);
								DEBUG << "Item " << idx << " in the pad is " << sv_type_by_id(old_sv->type) << " with addr " << (void *)old_sv->address;
								auto old = std::static_pointer_cast<pmat::sv_array>(old_sv);
								assert(old->type == sv_type_t::SVtARRAY);
								auto pad = std::make_shared<pmat::sv_pad>(*old);
								pad->set_cv(cv);
								replace_sv(old, pad);
								cv->pad_svs_.emplace_back(pad);
							}
							++idx;
						}
					}
				}
			}
		}

		void replace_sv(std::shared_ptr<pmat::sv> was, std::shared_ptr<pmat::sv> now) {
			assert(was->address == now->address);
			--sv_count_by_type_[was->type];
			sv_size_by_type_[was->type] -= was->size;

			++sv_count_by_type_[now->type];
			sv_size_by_type_[now->type] += now->size;

			sv_by_addr_[was->address] = now;
		}
	
		std::string sv_blessed_type(const pmat::sv &sv) const {
			auto base = sv_type_by_id(sv.type);
			if(sv.blessed == 0) return base;
			DEBUG << "We have " << (void *)sv.blessed << " as a blessed pointer";
			auto bs = std::static_pointer_cast<pmat::sv_stash>(sv_by_addr(sv.blessed));
			if(bs->type != sv_type_t::SVtSTASH) {
				ERROR << "We have something that has been blessed into something that isn't a stash: " << sv_type_by_id(bs->type);
				return base;
			}
			return base + "(" + bs->name + ")";
		}

		std::string sv_type_by_id(const sv_type_t &id) const {
			switch(id) {
			case sv_type_t::SVtEND: return u8"end of list";
			case sv_type_t::SVtGLOB: return u8"GLOB";
			case sv_type_t::SVtSCALAR: return u8"SCALAR";
			case sv_type_t::SVtREF: return u8"REF";
			case sv_type_t::SVtARRAY: return u8"ARRAY";
			case sv_type_t::SVtHASH: return u8"HASH";
			case sv_type_t::SVtSTASH: return u8"STASH";
			case sv_type_t::SVtCODE: return u8"CODE";
			case sv_type_t::SVtIO: return u8"IO";
			case sv_type_t::SVtLVALUE: return u8"LVALUE";
			case sv_type_t::SVtREGEXP: return u8"REGEXP";
			case sv_type_t::SVtFORMAT: return u8"FORMAT";
			case sv_type_t::SVtINVLIST: return u8"INVLIST";
			case sv_type_t::SVtPADNAMES: return u8"PADNAMES";
			case sv_type_t::SVtPADLIST: return u8"PADLIST";
			case sv_type_t::SVtPAD: return u8"PAD";
			case sv_type_t::SVtMAGIC: return u8"MAGIC";
			case sv_type_t::SVtUNKNOWN: return u8"UNKNOWN";
			default: return "unknown sv type";
			}
		}
	private:
		std::map<pmat::ptr_t, std::shared_ptr<pmat::sv>> sv_by_addr_;
		std::map<pmat::sv_type_t, size_t> sv_count_by_type_;
		std::map<std::string, size_t> sv_count_by_blessed_type_;
		std::map<pmat::sv_type_t, size_t> sv_size_by_type_;
		std::map<std::string, size_t> sv_size_by_blessed_type_;
		std::map<pmat::ptr_t, std::vector<pmat::ptr_t>> sv_blessed_pending_;
		size_t file_offset_;
	};
};


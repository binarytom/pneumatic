#ifndef DETAIL_H
#define DETAIL_H

// for the reader/writer
#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/system/system_error.hpp>

#include "pmat.h"
#include "net.h"
#include "Log.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <cxxabi.h>

#define DEBUG_TYPE(x) ([]() -> std::string { typedef void(*T)x; return debug_type<T>(T(), #x).result; })()

#define TRACE if(false) DEBUG

template<typename T>
struct debug_type
{
	template<typename U>
	debug_type(void(*)(U), const std::string& p_str)
	{
		std::string str(p_str.begin() + 1, p_str.end() - 1);
		str += " => ";
		char * name = 0;
		int status;
		name = abi::__cxa_demangle(typeid(U).name(), 0, 0, &status);
		if (name != 0) { str += name; }
		else { str += typeid(U).name(); }
		free(name);
		result = str;
	}
	std::string result;
};

/* Reader/writer support *******************************************************************/

namespace asio = boost::asio;
namespace fusion = boost::fusion;
namespace detail {

    namespace bs = boost::system;
	bs::system_error bad_message();

/*
 * The reader class is responsible for pulling data from the buffer and mapping it to the
 * appropriate types based on the Fusion definitions.
 *
 * Most methods will be called on a const reference, so we treat the following operations
 * as mutable (acceptable-on-const):
 *
 * * Buffer offset changes
 * * PMAT state changes
 * 
 */
class reader {
public:
    mutable asio::const_buffer buf_;
    mutable size_t offset_;
	pmat::state_t &pmat_state_;

    explicit reader(
		asio::const_buffer buf,
		pmat::state_t &state
	):buf_{std::move(buf)},
	  offset_{0},
	  pmat_state_(state)
    {
	}

	/** Move buffer offset forward - we don't update the buffer directly, but let this method do it for us */
	void forward(size_t v) const {
		buf_ = buf_ + v;
		offset_ += v;
		// TRACE << "Forward " << v << " - offset now " << offset_;
	}

    void operator()(double &val) const {
        val = *asio::buffer_cast<double const*>(buf_);
        forward(sizeof(double const *));
    }
    void operator()(float &val) const {
        val = *asio::buffer_cast<float const*>(buf_);
        forward(sizeof(float const*));
    }

	/**
	 * Deal with numeric types. We assume everything needs an endian flip, and let the ntoh
	 * templating sort out the details.
	 */
    template<class T>
    auto operator()(T & val) const ->
        typename std::enable_if<std::is_integral<T>::value>::type {
        val = *asio::buffer_cast<T const*>(buf_);
// TRACE << " Working with numeric value " << val;
        forward(sizeof(T));
    }

	/**
	 * If we have an enum, reduce it to a numeric type and try again.
	 */
    template<class T>
    auto operator()(T & val) const ->
        typename std::enable_if<std::is_enum<T>::value>::type {
        typename std::underlying_type<T>::type v;
        (*this)(v);
        val = static_cast<T>(v);
    }

	/**
	 * Numeric constants.
	 */
    template<class T, T v>
    void operator()(std::integral_constant<T, v>) const {
        typedef std::integral_constant<T, v> type;
        typename type::value_type val;
        (*this)(val);
		TRACE << "Constant lookup for " << DEBUG_TYPE((T)) << " - " << val;
        if (val != type::value)
            throw bad_message();
    }

#define ZEROSTR 0

	/**
	 * A string is expected to have a length prefix.
	 */
    void operator()(std::string& val) const {
#if ZEROSTR
        char ch;
		bool pending = true;
		val = u8"";
		while(pending) {
			(*this)(ch);
			TRACE << " Char is " << (uint32_t)ch;
			if(ch == 0) pending = false;
			else val += std::string { ch };
		}
		TRACE << " String " << val;
#else
        uint64_t length = 0;
        (*this)(length);
		// TRACE << " String length " << length;
		if(-1l == (int64_t)length) {
			return;
		}
		val = std::string(asio::buffer_cast<char const*>(buf_), length);
		forward(length);
		// TRACE << " String " << val;
#endif
    }

    void operator()(pmat::flags_t &val) const {
        (*this)(val.data);
		TRACE << "Applied flags: " << (uint32_t) val.data;
    }

    void operator()(pmat::ptr_t &val) const {
        uint64_t ptr = 0;
        (*this)(ptr);
		// ptr = net::hton(ptr);
		// TRACE << " ptr = " << ptr;
		val = reinterpret_cast<void *>(ptr);
		// TRACE << " val = " << val;
    }

    template<class T>
    void operator()(std::vector<T>& val) const {
        uint16_t length = 0;
        (*this)(length);
		// TRACE << "Reading " << length << " for u16 vector";
        for (; length; --length) {
            T v;
            (*this)(v);
            val.emplace_back(v);
        }
    }

    template<class T>
    void operator()(pmat::vec<uint64_t, T>& val) const {
        uint64_t length = 0;
        (*this)(length);
		TRACE << DEBUG_TYPE((T)) << " - read " << length << " items";
		auto items = val.items;
        for (; length; --length) {
            T v;
            (*this)(v);
            items.emplace_back(v);
        }
    }
    template<class T>
    void operator()(pmat::vec<uint32_t, T>& val) const {
        uint32_t length = 0;
        (*this)(length);
		TRACE << DEBUG_TYPE((T)) << " - read " << length << " items";
		auto items = val.items;
        for (; length; --length) {
            T v;
            (*this)(v);
            items.emplace_back(v);
        }
    }
    template<class T>
    void operator()(pmat::vec<uint8_t, T>& val) const {
        uint8_t length = 0;
        (*this)(length);
		TRACE << DEBUG_TYPE((T)) << " - read " << (uint32_t)length << " items";
        for (; length; --length) {
            T v;
            (*this)(v);
            val.items.emplace_back(v);
        }
    }

	/**
	 * Reading an entire SV is mildly complicated.
	 *
	 * We have a "magic" type which is handled differently - more on that later.
	 * Other types have a two-level size+count lookup:
	 * * Standard SV fields
	 * * Type-specific fields
	 */
    void
	operator()(pmat::sv& val) const {
		TRACE << "We have an SV starting at " << offset_;
		pmat::sv v;
		(*this)(v.type);
		switch(v.type) {
		case pmat::sv_type_t::SVtEND:
			TRACE << "Last entry";
			return;
			break;
		case pmat::sv_type_t::SVtMAGIC: {
			TRACE << "Magic?";
			pmat::magic_t m;
			(*this)(m.addr);
			(*this)(m.type);
			(*this)(m.flags);
			TRACE << "Address " << (void *) m.addr << " is type " << (uint32_t) m.type << " with flags " << (uint32_t) m.flags;
			(*this)(m.obj);
			(*this)(m.ptr);
			TRACE << "Obj = " << (void *) m.obj << ", ptr = " << (void *) m.obj;
			break;
		}
		default: {
			TRACE << " Have " << pmat_state_.types.size() << " to choose from";
			pmat::type base_type = pmat_state_.types[0];
			pmat::type spec_type = pmat_state_.types[(int) v.type];
			TRACE << "Scalar - will expect " << (uint32_t)base_type.headerlen << " bytes header, " << (uint32_t)base_type.nptrs << " pointers, " << (uint32_t)base_type.nstrs << " strings";
			const size_t total = (size_t)base_type.headerlen + sizeof(pmat::ptr_t) * (size_t)base_type.nptrs;
			TRACE << " Calculated " << total << " bytes, plus " << (uint32_t)base_type.nstrs << " strings";

			/* so we now have information about the total size of the serialized SV data, isolate it into
			 * a new const_buffer and work with that first
			 */
			// auto sv_buf = const_buffer { buf_, 

			/* Generic */
			(*this)(v.address);
			(*this)(v.refcnt);
			(*this)(v.size);
			(*this)(v.blessed);

			/* Specific */
			switch(v.type) {
			case pmat::sv_type_t::SVtSCALAR: {
				TRACE << "This is a scalar";
				pmat::sv_scalar scalar;
				(*this)(scalar.flags);
				if(scalar.flags & ~0x1f) {
					ERROR << "Invalid flags " << (int)scalar.flags;
				}
				TRACE << " flags (" << (int)scalar.flags << ") => " << (scalar.flags & 1 ? "has IV" : "no IV")
					<< (scalar.flags & 2 ? ", IV is UV" : "")
					<< (scalar.flags & 4 ? ", NV" : "")
					<< (scalar.flags & 8 ? ", STR" : "")
					<< (scalar.flags & 16 ? ", UTF8" : "");
				(*this)(scalar.iv);
				TRACE << "IV = " << scalar.iv;
				(*this)(scalar.nv);
				TRACE << "NV = " << scalar.nv;
				(*this)(scalar.pvlen);
				TRACE << "pvlen = " << scalar.pvlen;
				(*this)(scalar.ourstash);
				TRACE << "stash = " << (void *) scalar.ourstash;
				//if(scalar.flags & 0x08) {
				TRACE << "reading pv data";
				(*this)(scalar.pv);
				TRACE << "pv = " << scalar.pv;
				//}
				break;
			}
			case pmat::sv_type_t::SVtGLOB: {
				TRACE << "This is a glob";
				pmat::sv_glob glob;
				(*this)(glob);
				TRACE << " glob name " << glob.name << " from file " << std::string { glob.file };
				break;
			}
			case pmat::sv_type_t::SVtARRAY: {
				TRACE << "This be array";
				pmat::sv_array array;
				(*this)(array.count);
				(*this)(array.flags);
				TRACE << " has " << array.count << " elements with flags " << (int) array.flags;
				for(int i = 0; i < array.count; ++i) {
					pmat::ptr_t ptr;
					(*this)(ptr);
					array.elements.emplace_back(ptr);
				}
				break;
			}
			case pmat::sv_type_t::SVtHASH: {
				TRACE << "Hash time";
				pmat::sv_hash hash;
				(*this)(hash.count);
				(*this)(hash.backrefs);
				std::map<std::string, pmat::ptr_t> out;
				TRACE << "Has " << (int)hash.count << " key/value pairs";
				for(int i = 0; i < hash.count; ++i) {
					std::string k;
					pmat::ptr_t ptr;
					(*this)(k);
					(*this)(ptr);
					out[k] = ptr;
					TRACE << " key " << k << " == " << (void *) ptr;
				}
				break;
			}
			case pmat::sv_type_t::SVtSTASH: {
				TRACE << "Stash time";
				pmat::sv_stash stash;
				(*this)(stash.count);
				(*this)(stash.backrefs);
				(*this)(stash.mro_linear_all);
				(*this)(stash.mro_linear_current);
				(*this)(stash.mro_nextmethod);
				(*this)(stash.mro_isa);
				(*this)(stash.name);
				std::map<std::string, pmat::ptr_t> out;
				TRACE << stash.name << " has " << (int)stash.count << " key/value pairs";
				for(int i = 0; i < stash.count; ++i) {
					std::string k;
					pmat::ptr_t ptr;
					(*this)(k);
					(*this)(ptr);
					out[k] = ptr;
					TRACE << " key " << k << " == " << (void *) ptr;
				}
				break;
			}
			case pmat::sv_type_t::SVtREF: {
				TRACE << "REF time";
				pmat::sv_ref ref;
				(*this)(ref);
				if(ref.flags & 1) {
					TRACE << "This ref is weak";
				}
				break;
			}
			case pmat::sv_type_t::SVtCODE: {
				TRACE << "We have code";
				pmat::sv_code code;
				(*this)(code.line);
				(*this)(code.flags);
				TRACE << " has " << code.line << " with flags " << (int) code.flags;
				(*this)(code.op_root);
				(*this)(code.stash);
				(*this)(code.glob);
				(*this)(code.outside);
				(*this)(code.padlist);
				(*this)(code.constval);
				(*this)(code.file);
				TRACE << " file " << code.file;
				pmat::sv_code_type_t type;
				(*this)(type);
				while(type != pmat::sv_code_type_t::SVCtEND) {
					TRACE << "Type is " << (int)type;
					switch(type) {
					case pmat::sv_code_type_t::SVCtCONSTSV: {
						pmat::sv_code_constsv constsv;
						(*this)(constsv);
						TRACE << "Had constsv " << (void*)constsv.target_sv;
						break;
					}
					case pmat::sv_code_type_t::SVCtCONSTIX: {
						pmat::sv_code_constix constix;
						(*this)(constix);
						TRACE << "Had constsv " << constix.padix;
						break;
					}
					case pmat::sv_code_type_t::SVCtGVSV: {
						pmat::sv_code_gvsv gvsv;
						(*this)(gvsv);
						TRACE << "Had GVSV " << (void *)gvsv.target_sv;
						break;
					}
					case pmat::sv_code_type_t::SVCtGVIX: {
						pmat::sv_code_gvix gvix;
						(*this)(gvix);
						TRACE << "Had GVIX " << gvix.padix;
						break;
					}
					case pmat::sv_code_type_t::SVCtPADNAMES: {
						pmat::sv_code_padnames padnames;
						(*this)(padnames);
						TRACE << "Had padnames " << (void *)padnames.names;
						break;
					}
					case pmat::sv_code_type_t::SVCtPAD: {
						pmat::sv_code_pad pad;
						(*this)(pad);
						TRACE << "Had depth " << pad.depth << " pad " << pad.pad;
						break;
					}
					default: ERROR << "unknwon thing";
					}
									// SVCtPADNAME = 5,
									// SVCtPADSV = 6,
					(*this)(type);
				}
				break;
			}
			case pmat::sv_type_t::SVtIO: {
				TRACE << "IO";
				pmat::sv_io io;
				(*this)(io);
				break;
			}
			case pmat::sv_type_t::SVtLVALUE: {
				TRACE << "LVALUE";
				pmat::sv_lvalue lv;
				(*this)(lv);
				break;
			}
			case pmat::sv_type_t::SVtREGEXP: {
				TRACE << "Regexp";
				/*
				pmat::sv_re re;
				(*this)(re);
				*/
				break;
			}
			case pmat::sv_type_t::SVtFORMAT: {
				TRACE << "Format";
				/*
				pmat::sv_format format;
				(*this)(format);
				*/
				break;
			}
			case pmat::sv_type_t::SVtINVLIST: {
				TRACE << "Invlist";
				/*
				pmat::sv_invlist invlist;
				(*this)(invlist);
				*/
				break;
			}
			case pmat::sv_type_t::SVtUNKNOWN: {
				TRACE << "Unknown type, skipping by size field " << v.size;
				break;
			}
			default:
				ERROR << "Unknown type " << (uint32_t) v.type;
				for(int i = 0; i < (uint32_t)spec_type.headerlen; ++i) {
					char ch;
					(*this)(ch);
				}
				for(int i = 0; i < (uint32_t)spec_type.nptrs; ++i) {
					pmat::ptr_t ptr;
					(*this)(ptr);
				}
				for(int i = 0; i < (uint32_t)spec_type.nstrs; ++i) {
					std::string str;
					(*this)(str);
				}
				break;
			}
			break;
		}
		}
		val = v;
		pmat_state_.add_sv(v);
	}

    void operator()(pmat::type& v) const {
		(*this)(v.headerlen);
		(*this)(v.nptrs);
		(*this)(v.nstrs);
		TRACE << "Type: header " << (uint32_t) v.headerlen << ", nptrs " << (uint32_t) v.nptrs << ", nstrs " << (uint32_t) v.nstrs;
		pmat_state_.types.push_back(v);
		TRACE << "PMAT state now has " << pmat_state_.types.size() << " types - " << (void*)(&pmat_state_);
	}

    void operator()(pmat::heap& val) const {
		TRACE << "Starting on the heap";
		std::vector<pmat::sv> items;
		bool active = true;
		while(active) {
			pmat::sv item;
			(*this)(item);
			if(item.type == pmat::sv_type_t::SVtEND) {
				TRACE << "Item type 0 == end";
				active = false;
			} else {
				TRACE << "Item type " << (uint32_t) item.type << ", size was " << item.size << " at address " << item.address;
			}
        }
    }

#if(0)
    void operator()(boost::posix_time::ptime& val) const {
        std::string iso_str;
        (*this)(iso_str);
        val = boost::posix_time::from_iso_string(iso_str);
    }
    template<class K, class V>
    void operator()(boost::container::flat_map<K,V>& val) const {
        uint16_t length = 0;
        (*this)(length);
        for (; length; --length) {
            K key;
            (*this)(key);
            V value;
            (*this)(value);
            val.emplace(key,value);
        }
    }
    void operator()(example::opt_fields&) const {
        example::opt_fields::value_type val;
        (*this)(val);
        opts_ = example::opt_fields::bits_type(val);
    }

    template<class T, size_t N>
    void operator()(example::optional_field<T, N> & val) const {
        if (!opts_)
            throw bad_message();
        if ((*opts_)[N]) {
            T v;
            (*this)(v);
            val = example::optional_field<T, N>(std::move(v));
        }
    }
    template<class T>
    void operator()(example::lazy<T> & val) const {
        val = example::lazy<T>(buf_);
        forward(val.buffer_size());
    }

    template<class T>
    void operator()(example::lazy_range<T> & val) const {
        uint16_t length;
        (*this)(length);
        val = example::lazy_range<T>(length, buf_);
    }
#endif

    template<class T>
    auto operator()(T & val) const ->
    typename std::enable_if<boost::fusion::traits::is_sequence<T>::value>::type {
		TRACE << DEBUG_TYPE((T)) << " - iteration";
        boost::fusion::for_each(val, *this);
    }
};

/**
 * When reading, we provide the base object and the state structure.
 * The base object is the one we're populating - header, roots, stack etc.
 * - and the state object acts as a manager for the updates to internal state.
 */
template<typename T>
std::pair<T, asio::const_buffer> read(asio::const_buffer b, pmat::state_t &s)
{
	auto r = reader { std::move(b), s };
	T res;
	(r)(res);
	return std::make_pair(res, r.buf_);
}

}

#endif

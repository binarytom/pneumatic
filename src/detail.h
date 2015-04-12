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

#if(1)
namespace asio = boost::asio;
namespace fusion = boost::fusion;
namespace detail {

    namespace bs = boost::system;
	bs::system_error bad_message();

class reader {
public:
    mutable asio::const_buffer buf_;
    mutable size_t offset_;
	mutable pmat::state_t pmat_state_;

    // mutable boost::optional<example::opt_fields::bits_type> opts_;

    explicit reader(
		asio::const_buffer buf
	):buf_{std::move(buf)},
	  offset_{0}
    {
	}

	void forward(size_t v) const { buf_ = buf_ + v; offset_ += v; DEBUG << "Forward " << v << " - offset now " << offset_; }

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
        // val = net::ntoh(*asio::buffer_cast<T const*>(buf_));
        val = *asio::buffer_cast<T const*>(buf_);
// DEBUG << " Working with numeric value " << val;
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
		DEBUG << "Constant lookup for " << DEBUG_TYPE((T)) << " - " << val;
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
			DEBUG << " Char is " << (uint32_t)ch;
			if(ch == 0) pending = false;
			else val += std::string { ch };
		}
		DEBUG << " String " << val;
#else
        uint64_t length = 0;
        (*this)(length);
		DEBUG << " String length " << length;
		if(length > 0x00800000) {
			val = "<error, " + std::to_string(length) + " too large>";
			DEBUG << "String overflow " << val;
			return;
		}
		val = std::string(asio::buffer_cast<char const*>(buf_), length);
		forward(length);
		DEBUG << " String " << val;
#endif
    }

    void operator()(pmat::flags_t &val) const {
        uint8_t flags = 0;
        (*this)(flags);
		val.big_endian = flags & 0x01;
		val.integer_64 = flags & 0x02;
		val.pointer_64 = flags & 0x04;
		val.float_64 = flags & 0x08;
		val.threads = flags & 0x10;
		DEBUG << "Applied flags: " << (uint32_t) flags;
    }

    void operator()(pmat::ptr_t &val) const {
        uint64_t ptr = 0;
        (*this)(ptr);
		// ptr = net::hton(ptr);
		DEBUG << " ptr = " << ptr;
		val = reinterpret_cast<void *>(ptr);
		DEBUG << " val = " << val;
    }

    template<class T>
    void operator()(std::vector<T>& val) const {
        uint16_t length = 0;
        (*this)(length);
		DEBUG << "Reading " << length << " for u16 vector";
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
		DEBUG << DEBUG_TYPE((T)) << " - read " << length << " items";
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
		DEBUG << DEBUG_TYPE((T)) << " - read " << length << " items";
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
		DEBUG << DEBUG_TYPE((T)) << " - read " << (uint32_t)length << " items";
        for (; length; --length) {
            T v;
            (*this)(v);
            val.items.emplace_back(v);
        }
    }

    void operator()(pmat::sv& val) const {
		DEBUG << "We have an SV";
		pmat::sv v;
		(*this)(v.type);
		switch(v.type) {
		case pmat::sv_type_t::SVtEND:
			DEBUG << "Last entry";
			return;
			break;
		case pmat::sv_type_t::SVtMAGIC: {
			DEBUG << "Magic?";
			pmat::magic_t m;
			(*this)(m.addr);
			(*this)(m.type);
			(*this)(m.flags);
			DEBUG << "Address " << (void *) m.addr << " is type " << (uint32_t) m.type << " with flags " << (uint32_t) m.flags;
			(*this)(m.obj);
			(*this)(m.ptr);
			DEBUG << "Obj = " << (void *) m.obj << ", ptr = " << (void *) m.obj;
			break;
		}
		default: {
			pmat::type base_type = pmat_state_.types[0];
			pmat::type spec_type = pmat_state_.types[(int) v.type];
			DEBUG << "Scalar - will expect " << (uint32_t)base_type.headerlen << " bytes header, " << (uint32_t)base_type.nptrs << " pointers, " << (uint32_t)base_type.nstrs << " strings";
			DEBUG << "then " << (uint32_t)spec_type.headerlen << " bytes header, " << (uint32_t)spec_type.nptrs << " pointers, " << (uint32_t)spec_type.nstrs << " strings";

			/* Generic */
			(*this)(v.address);
			(*this)(v.refcnt);
			(*this)(v.size);
			(*this)(v.blessed);

			/* Specific */
			switch(v.type) {
			case pmat::sv_type_t::SVtSCALAR: {
				DEBUG << "This is a scalar";
				pmat::sv_scalar scalar;
				(*this)(scalar.flags);
				if(scalar.flags & ~0x1f) {
					ERROR << "Invalid flags " << (int)scalar.flags;
				}
				DEBUG << " flags (" << (int)scalar.flags << ") => " << (scalar.flags & 1 ? "has IV" : "no IV")
					<< (scalar.flags & 2 ? ", IV is UV" : "")
					<< (scalar.flags & 4 ? ", NV" : "")
					<< (scalar.flags & 8 ? ", STR" : "")
					<< (scalar.flags & 16 ? ", UTF8" : "");
				(*this)(scalar.iv);
				DEBUG << "IV = " << scalar.iv;
				(*this)(scalar.nv);
				DEBUG << "NV = " << scalar.nv;
				(*this)(scalar.pvlen);
				DEBUG << "pvlen = " << scalar.pvlen;
				(*this)(scalar.ourstash);
				DEBUG << "stash = " << (void *) scalar.ourstash;
				//if(scalar.flags & 0x08) {
				DEBUG << "reading pv data";
				(*this)(scalar.pv);
				DEBUG << "pv = " << scalar.pv;
				//}
				break;
			}
			case pmat::sv_type_t::SVtGLOB: {
				DEBUG << "This is a glob";
				pmat::sv_glob glob;
				(*this)(glob);
				DEBUG << " glob name " << glob.name << " from file " << std::string { glob.file };
				break;
			}
			case pmat::sv_type_t::SVtARRAY: {
				DEBUG << "This is a array";
				pmat::sv_array array;
				(*this)(array.count);
				(*this)(array.flags);
				DEBUG << " has " << array.count << " elements with flags " << (int) array.flags;
				for(int i = 0; i < array.count; ++i) {
					pmat::ptr_t ptr;
					(*this)(ptr);
					array.elements.emplace_back(ptr);
				}
				break;
			}
			case pmat::sv_type_t::SVtCODE: {
				DEBUG << "We have code";
				pmat::sv_code code;
				(*this)(code.line);
				(*this)(code.flags);
				DEBUG << " has " << code.line << " with flags " << (int) code.flags;
				(*this)(code.op_root);
				(*this)(code.stash);
				(*this)(code.glob);
				(*this)(code.outside);
				(*this)(code.padlist);
				(*this)(code.constval);
				(*this)(code.file);
				DEBUG << " file " << code.file;
				pmat::sv_code_type_t type;
				(*this)(type);
				while(type != pmat::sv_code_type_t::SVCtEND) {
					DEBUG << "Type is " << (int)type;
					switch(type) {
					case pmat::sv_code_type_t::SVCtCONSTSV: {
						pmat::sv_code_constsv constsv;
						(*this)(constsv);
						DEBUG << "Had constsv " << (void*)constsv.target_sv;
						break;
					}
					case pmat::sv_code_type_t::SVCtCONSTIX: {
						pmat::sv_code_constix constix;
						(*this)(constix);
						DEBUG << "Had constsv " << constix.padix;
						break;
					}
					case pmat::sv_code_type_t::SVCtGVSV: {
						pmat::sv_code_gvsv gvsv;
						(*this)(gvsv);
						DEBUG << "Had GVSV " << (void *)gvsv.target_sv;
						break;
					}
					case pmat::sv_code_type_t::SVCtGVIX: {
						pmat::sv_code_gvix gvix;
						(*this)(gvix);
						DEBUG << "Had GVIX " << gvix.padix;
						break;
					}
					case pmat::sv_code_type_t::SVCtPADNAMES: {
						pmat::sv_code_padnames padnames;
						(*this)(padnames);
						DEBUG << "Had padnames " << (void *)padnames.names;
						break;
					}
					case pmat::sv_code_type_t::SVCtPAD: {
						pmat::sv_code_pad pad;
						(*this)(pad);
						DEBUG << "Had depth " << pad.depth << " pad " << pad.pad;
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
			case pmat::sv_type_t::SVtUNKNOWN: {
				DEBUG << "Unknown type, skipping by size field " << v.size;
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
	}

    void operator()(pmat::type& v) const {
		(*this)(v.headerlen);
		(*this)(v.nptrs);
		(*this)(v.nstrs);
		DEBUG << "Type: header " << (uint32_t) v.headerlen << ", nptrs " << (uint32_t) v.nptrs << ", nstrs " << (uint32_t) v.nstrs;
		pmat_state_.types.push_back(v);
	}

    void operator()(pmat::heap& val) const {
		DEBUG << "Starting on the heap";
		std::vector<pmat::sv> items;
		bool active = true;
		while(active) {
			pmat::sv item;
			(*this)(item);
			if(item.type == pmat::sv_type_t::SVtEND) {
				DEBUG << "Item type 0 == end";
				active = false;
			} else {
				DEBUG << "Item type " << (uint32_t) item.type << ", size was " << item.size << " at address " << item.address;
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
		DEBUG << DEBUG_TYPE((T)) << " - iteration";
        boost::fusion::for_each(val, *this);
    }

};

/*
template<typename T>
T read(asio::const_buffer b) {
	reader r(std::move(b));
	T res;
	fusion::for_each(res, r);
	return res;
}
*/

template<typename T>
std::pair<T, asio::const_buffer> read(asio::const_buffer b) {
	reader r(std::move(b));
	T res;
	fusion::for_each(res, r);
	return std::make_pair(res, r.buf_);
}

}
#endif

#endif

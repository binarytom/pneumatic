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
        val = net::ntoh(*asio::buffer_cast<T const*>(buf_));
        buf_ = buf_ + sizeof(T);
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
        if (val != type::value)
            throw bad_message();
    }

	/**
	 * A string is expected to have a length prefix.
	 */
    void operator()(std::string& val) const {
        uint16_t length = 0;
        (*this)(length);
        val = std::string(asio::buffer_cast<char const*>(buf_), length);
        buf_ = buf_ + length;
    }

    void operator()(pmat::flags_t &val) const {
        uint8_t flags = 0;
        (*this)(flags);
		val.big_endian = flags & 0x01;
		val.integer_64 = flags & 0x02;
		val.pointer_64 = flags & 0x04;
		val.float_64 = flags & 0x08;
		val.threads = flags & 0x10;
    }

#if(0)
    void operator()(boost::posix_time::ptime& val) const {
        std::string iso_str;
        (*this)(iso_str);
        val = boost::posix_time::from_iso_string(iso_str);
    }
#endif
    template<class T>
    void operator()(std::vector<T>& val) const {
        uint16_t length = 0;
        (*this)(length);
        for (; length; --length) {
            T v;
            (*this)(v);
            val.emplace_back(v);
        }
    }

#if(0)
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
#endif

#if(0)
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
#endif
    template<class T>
    auto operator()(T & val) const ->
    typename std::enable_if<boost::fusion::traits::is_sequence<T>::value>::type {
        boost::fusion::for_each(val, *this);
    }

#if(0)
    template<class T>
    void operator()(example::lazy<T> & val) const {
        val = example::lazy<T>(buf_);
        buf_ = buf_ + val.buffer_size();
    }

    template<class T>
    void operator()(example::lazy_range<T> & val) const {
        uint16_t length;
        (*this)(length);
        val = example::lazy_range<T>(length, buf_);
    }
#endif
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

/**
 * @file Log.h
 *
 */
#ifndef LOG_H
#define LOG_H

#include <boost/log/trivial.hpp>

#define TRACE BOOST_LOG_TRIVIAL(trace)
#define DEBUG BOOST_LOG_TRIVIAL(debug)
#define INFO BOOST_LOG_TRIVIAL(info)
#define WARN BOOST_LOG_TRIVIAL(warn)
#define ERROR BOOST_LOG_TRIVIAL(error)
#define FATAL BOOST_LOG_TRIVIAL(fatal)

#endif // LOG_H

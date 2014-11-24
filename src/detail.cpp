#include "detail.h"

namespace detail {
	boost::system::system_error bad_message() {
		return boost::system::system_error(boost::system::errc::make_error_code(boost::system::errc::bad_message));
	}
}


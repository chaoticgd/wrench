#ifndef ALGORITHM_UTIL_H
#define ALGORITHM_UTIL_H

#include <algorithm>

template <typename T_output, typename T_input, typename T_op>
T_output transform_all(T_input in, T_op f) {
	T_output result(std::distance(in.end() - in.begin()));
	std::transform(in.begin(), in.end(), result.begin(), f);
	return result;
}

#endif

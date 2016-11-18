/*
	Binary calculation functions
*/

#pragma once

#include <string>
#include <type_traits>

template<
	typename int_t,
	class = std::enable_if<std::is_integral<int_t>::value>::type
>
static std::string decimalToBinary(int_t x)
{
	using namespace std;

	string buf;
	int sz = (sizeof(int_t) * CHAR_BIT);
	buf.reserve(sz);

	for (int idx = (sz - 1); idx >= 0; idx--)
	{
		if (x & (1 << idx))
		{
			buf += '1';
		}
		else
		{
			buf += '0';
		}
	}

	return buf;
}

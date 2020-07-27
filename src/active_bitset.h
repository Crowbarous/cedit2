#ifndef ACTIVE_BITSET_H
#define ACTIVE_BITSET_H

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

template <typename T>
struct active_bitset {
	static_assert(std::is_unsigned_v<T>,
		"Underlying type for active_bitset must be integral unsigned");
	static constexpr int bits_per_elem = std::numeric_limits<T>::digits;
	static constexpr int bits_low = std::log2(bits_per_elem);
	static_assert((bits_per_elem & (bits_per_elem-1)) == 0,
		"Bits per element of active_bitset must be a power of 2");

	std::vector<T> vals = { 0 };
	int min_cleared = 0;

	int set_first_cleared ()
	{
		const int result = min_cleared;
		int high = min_cleared >> bits_low;
		int low = min_cleared & ((1 << bits_low) - 1);

		vals[high] |= (T{1} << low);

		for (; high < vals.size(); high++) {
			if (~vals[high] == 0)
				continue;

			T lowest_bit = (~vals[high]) & (vals[high] + 1);
			low = std::log2(lowest_bit);
			min_cleared = (high << bits_low) | low;
			return result;
		}

		vals.push_back(0);
		min_cleared = high << bits_low;
		return result;
	}

	void first_n_only (int n)
	{
		vals.clear();
		for (; n >= bits_per_elem; n -= bits_per_elem)
			vals.push_back(~T{0});
		if (n > 0)
			vals.push_back((T{1} << n) - 1);
	}

	bool is_set (int index) const
	{
		int high = index >> bits_low;
		if (high >= vals.size())
			return false;
		int low = index & ((1 << bits_low) - 1);
		return (vals[high] & (1 << low)) != 0;
	}

	void clear (int index)
	{
		int high = index >> bits_low;
		if (high >= vals.size())
			return;

		int low = index & ((1 << bits_low) - 1);
		vals[high] &= ~T{T{1} << low};

		if (index < min_cleared)
			min_cleared = index;

		while (vals.size() > 1 && vals.back() == 0)
			vals.pop_back();
	}

	void clear_all ()
	{
		vals = { 0 };
		min_cleared = 0;
	}

	void debug_pretty_print () const
	{
		for (T v: vals) {
			for (int i = 0; i < bits_per_elem; i++, v /= 2) {
				fputc((v % 2) ? 'X' : '.', stderr);
				if (i % 8 == 7)
					fputc(' ', stderr);
			}
			fputc('\n', stderr);
		}
	}
};

using active_bitset32 = active_bitset<uint32_t>;
using active_bitset64 = active_bitset<uint64_t>;

#endif // ACTIVE_BITSET_H

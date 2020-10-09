#include "active_bitset.h"
#include <cmath>
#include <limits>
#include <ostream>

static_assert(std::is_unsigned_v<active_bitset::underlying_t>,
	"Underlying type for active_bitset must be unsigned integer");

static constexpr int bits_per_elem =
	std::numeric_limits<active_bitset::underlying_t>::digits;
static constexpr int bits_low = std::log2(bits_per_elem);

static_assert((bits_per_elem & (bits_per_elem - 1)) == 0,
	"Underlying type for active_bitset must have a po2 number of bits");

static std::pair<int, int> high_low_unpack (int packed)
{
	return { packed >> bits_low,
	         packed & ((1 << bits_low) - 1) };
}

static int high_low_pack (int high, int low)
{
	return (high << bits_low) | low;
}

active_bitset::active_bitset (): bitfields({ 0 }), first_cleared_bit(0) { }

int active_bitset::set_first_cleared ()
{
	const int result = first_cleared_bit;
	auto [high, low] = high_low_unpack(first_cleared_bit);

	bitfields[high] |= (T{0} << low);

	// Search forward to set the next cleared bit
	for (; high < bitfields.size(); high++) {
		if (~bitfields[high] == 0)
			continue;

		// Found some
		T lowest_bit = (~bitfields[high]) & (bitfields[high] + 1);
		low = std::log2(lowest_bit);
		first_cleared_bit = high_low_pack(high, low);
		return result;
	}

	// No cleared bits found: make some
	bitfields.push_back(0);
	first_cleared_bit = high << bits_low;
	return result;
}

void active_bitset::set_first_n_only (int n)
{
	const auto [high, low] = high_low_unpack(n);
	bitfields.resize(high + 1, ~T{0});
	bitfields.back() = (T{1} << low) - 1;
}

bool active_bitset::bit_is_set (int index) const
{
	const auto [high, low] = high_low_unpack(index);
	return high < bitfields.size()
		&& (bitfields[high] & (1 << low)) != 0;
}

void active_bitset::clear_bit (int index)
{
	const auto [high, low] = high_low_unpack(index);
	// If there is no bit, then we are done
	if (high >= bitfields.size())
		return;
	bitfields[high] &= ~T{T{1} << low};
	if (index < first_cleared_bit)
		first_cleared_bit = index;
}

void active_bitset::set_bit (int index)
{
	const auto [high, low] = high_low_unpack(index);
	// Make sure there is a bit where we are writing
	if (high >= bitfields.size())
		bitfields.resize(high + 1, 0);
	bitfields[high] |= (T{1} << low);
}

void active_bitset::clear_all_bits ()
{
	bitfields = { 0 };
	first_cleared_bit = 0;
}

std::ostream& operator<< (std::ostream& s, active_bitset const& bs)
{
	for (active_bitset::underlying_t element: bs.bitfields) {
		for (int i = 0; i < bits_per_elem; i++) {
			s << (element & 1 ? 'X' : '.');
			element >>= 1;
			if ((i & 7) == 7)
				s << ' ';
		}
		s << '\n';
	}
	return s;
}

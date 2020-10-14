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
	const int result = this->first_cleared_bit;
	auto [high, low] = high_low_unpack(this->first_cleared_bit);

	this->bitfields[high] |= (T{1} << low);

	// Search forward to set the next cleared bit
	for (; high < bitfields.size(); high++) {
		if (~bitfields[high] == 0)
			continue;

		// Found some
		const T bitfd = this->bitfields[high];
		const T lowest_bit = (~bitfd) & (bitfd + 1);
		low = std::log2(lowest_bit);
		this->first_cleared_bit = high_low_pack(high, low);
		return result;
	}

	// No cleared bits found: make some
	this->bitfields.push_back(0);
	this->first_cleared_bit = high << bits_low;
	return result;
}

void active_bitset::set_first_n_only (int n)
{
	const auto [high, low] = high_low_unpack(n);
	this->bitfields = std::vector<T>(high + 1, ~T{0});
	this->bitfields.back() = (T{1} << low) - 1;
	this->first_cleared_bit = n;
}

bool active_bitset::bit_is_set (int index) const
{
	const auto [high, low] = high_low_unpack(index);
	return high < this->bitfields.size()
		&& (this->bitfields[high] & (1 << low)) != 0;
}

void active_bitset::clear_bit (int index)
{
	const auto [high, low] = high_low_unpack(index);
	// If there is no bit, then we are done
	if (high >= this->bitfields.size())
		return;
	this->bitfields[high] &= ~T{T{1} << low};
	if (index < this->first_cleared_bit)
		this->first_cleared_bit = index;
}

void active_bitset::set_bit (int index)
{
	const auto [high, low] = high_low_unpack(index);
	// Make sure there is a bit where we are writing
	if (high >= this->bitfields.size())
		this->bitfields.resize(high + 1, 0);
	this->bitfields[high] |= (T{1} << low);
}

void active_bitset::clear_all_bits ()
{
	this->bitfields = { 0 };
	this->first_cleared_bit = 0;
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

#ifndef NDEBUG

#include <iostream>

void active_bitset_interactive_test ()
{
	std::cout <<
		"Testing active_bitset:\n"
		"0 to quit\n"
		"1 to set first cleared bit\n"
		"2 <N> to clear Nth bit\n"
		"3 <N> to set first N bits only\n"
		"4 to clear all bits\n";

	active_bitset bs;
	while (true) {
		int action, n;
		std::cin >> action;

		switch (action) {
		default:
			return;
		case 1:
			std::cout << bs.set_first_cleared() << '\n';
			break;
		case 2:
			std::cin >> n;
			bs.clear_bit(n);
			break;
		case 3:
			std::cin >> n;
			bs.set_first_n_only(n);
			break;
		case 4:
			bs.clear_all_bits();
			break;
		}

		std::cout << bs << std::flush;
	}
}
#endif

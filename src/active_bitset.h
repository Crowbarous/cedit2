#ifndef ACTIVE_BITSET_H
#define ACTIVE_BITSET_H

#include <vector>
#include <cstdio>

class active_bitset {
private:
	using T = size_t; /* Whatever the machine word is */
	std::vector<T> bitfields;
	int first_cleared_bit;
	int total_set_bits;
public:
	using underlying_t = T;

	active_bitset ();

	/* Find the first cleared bit, set it, and return its index */
	int set_first_cleared ();

	/* Make the `n` first bits the only ones set */
	void set_first_n_only (int n);

	bool bit_is_set (int index) const;
	void clear_bit (int index);
	void clear_all_bits ();
	void set_bit (int index);

	int popcount () const;

	/* Pretty-print over multiple lines (!) for debug */
	void dump_info (FILE* outstream) const;
};

#endif /* ACTIVE_BITSET_H */

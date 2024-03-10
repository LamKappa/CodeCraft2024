#include <cstring>
#include <utility>

#include "Config.h"

template<long SIZE>
struct Bitset {
    using VALUE_TYPE = u64;
    static constexpr long BIT_PER_UNIT = sizeof(VALUE_TYPE) * 8;
    VALUE_TYPE bits[SIZE / BIT_PER_UNIT + 1];

    [[nodiscard]] inline bool test(long p) const {
        return static_cast<bool>(bits[p / BIT_PER_UNIT] & (1ull << (p % BIT_PER_UNIT)));
    }
    inline void set(long p) {
        bits[p / BIT_PER_UNIT] |= (1ull << (p % BIT_PER_UNIT));
    }
    inline void set(long l, long r) {
        if(l >= r) return;
        r--;
        auto L = l / BIT_PER_UNIT, R = r / BIT_PER_UNIT;
        if(L == R) {
            bits[L] |= ((1ull << (r % BIT_PER_UNIT + 1u)) - 1) ^ ((1ull << (l % BIT_PER_UNIT)) - 1);
        } else {
            if(L + 1 <= R - 1) {
                std::memset(bits + L + 1, 0xff, (R - L - 1) * sizeof(VALUE_TYPE));
            }
            bits[L] |= ~((1ull << (l % BIT_PER_UNIT)) - 1);
            bits[R] |= ((1ull << (r % BIT_PER_UNIT + 1u)) - 1);
        }
    }
    inline void reset(long p) {
        bits[p / BIT_PER_UNIT] &= ~(1ull << (p % BIT_PER_UNIT));
    }
    inline void reset(long l, long r) {
        if(l >= r) return;
        r--;
        auto L = l / BIT_PER_UNIT, R = r / BIT_PER_UNIT;
        if(L == R) {
            bits[L] &= ~(((1ull << (r % BIT_PER_UNIT + 1u)) - 1) ^ ((1ull << (l % BIT_PER_UNIT)) - 1));
        } else {
            if(L + 1 <= R - 1) {
                std::memset(bits + L + 1, 0x00, (R - L - 1) * sizeof(VALUE_TYPE));
            }
            bits[L] &= ((1ull << (l % BIT_PER_UNIT)) - 1);
            bits[R] &= ~((1ull << (r % BIT_PER_UNIT + 1u)) - 1);
        }
    }
};
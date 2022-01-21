#include "wrapping_integers.hh"
#include <bits/stdint-uintn.h>
#include <cstdint>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t res = isn.raw_value();
    uint64_t mod = (1ull) << 32;
    res = (res + n) % (mod);
    return WrappingInt32{static_cast<uint32_t>(res)};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t step = (1ull) << 32;
    uint32_t raw_n = n.raw_value(), raw_isn = isn.raw_value();
    uint64_t diff = raw_n >= raw_isn ? raw_n - raw_isn : (step - raw_isn) + raw_n;
    uint64_t left = checkpoint & (0xFFFFFFFF00000000), l_ans = left + diff; // bitwise opt
    uint64_t right = left + step, r_ans = right + diff;
    if (l_ans >= checkpoint) {
        if (l_ans < step) return l_ans;
        l_ans -= step;
        r_ans -= step;
    }
    uint64_t left_distance = checkpoint - l_ans;
    uint64_t right_distance = r_ans - checkpoint;
    return left_distance < right_distance ? l_ans : r_ans;
}

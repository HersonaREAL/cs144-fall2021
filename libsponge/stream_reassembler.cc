#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {

}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= m_eofPos) {
        fflush();
        return;
    }
    
    //TODO overlap
    size_t rlen = data.size();
    size_t ridx = index;
    size_t offset = 0;
    bool recursion = false;
    size_t recursion_idx = index;
    size_t recursion_off = 0;
    auto it = m_substrs.lower_bound(index);
    if (it != m_substrs.end() && it != m_substrs.begin()) {
        //cut tail
        if (ridx + rlen > it->first) {
            if ((ridx + rlen) > (it->first + it->second.size())) {
                // recursive
                recursion = true;
                recursion_idx = it->first + it->second.size();
                recursion_off = recursion_idx - ridx;
            }
            rlen = it->first - ridx;
        }

        //cut head
        --it;
        if (it->first + it->second.size() > ridx) {
            ridx = it->first + it->second.size();
            if (rlen < (ridx - index)) return;
            rlen -= ridx - index;
            offset += ridx - index;
        }
    } else if (it == m_substrs.begin() && it != m_substrs.end()) {
        // data will insert to the map begin, cut tail of data if necessary 
        if ((ridx + rlen) > (it->first + it->second.size())) {
                // recursive
                recursion = true;
                recursion_idx = it->first + it->second.size();
                recursion_off = recursion_idx - ridx;
        }

        rlen = ridx + rlen > it->first ? it->first - ridx : rlen;
        if (ridx < m_pos) {
            if (rlen < (m_pos - ridx)) return;
            rlen -= m_pos - ridx;
            offset += m_pos - ridx;
            ridx = m_pos;
        }
    } else if (it == m_substrs.end() && !m_substrs.empty()) {
        // data will insert to the map end, cut head of data if necessary
        --it;
        if (it->first + it->second.size() > ridx) {
            ridx = it->first + it->second.size();
            if (rlen < (ridx - index)) return;
            rlen -= ridx - index;
            offset += ridx - index;
        }
    } else if (m_substrs.empty()) {
        // insert to empty map, cut head if necessary
        if (ridx < m_pos) {
            if (rlen < (m_pos - ridx)) return;
            rlen -= m_pos - ridx;
            offset += m_pos - ridx;
            ridx = m_pos;
        }
    }

    // can not store this chunk
    if ((unassembled_bytes() + _output.buffer_size() + rlen > _capacity)) {
        // TODO adjust rlen
        rlen = _capacity - (unassembled_bytes() + _output.buffer_size());
        recursion = false;
        if (rlen == 0) {
            fflush();
            return;
        }
    }

    //rlen limit int [assembled index, assembled index + capacity)
    if ((ridx + rlen) > (m_pos - _output.buffer_size() + _capacity)) {
        recursion = false;
        rlen = (m_pos - _output.buffer_size() + _capacity) - ridx;
        if (rlen == 0) {
            fflush();
            return;
        }
    }

    // update substr
    if (rlen > 0) {
        string &&tmp = data.substr(offset, rlen);
        m_substrSize += rlen;
        m_substrs[ridx] = std::move(tmp);
    }

    if (recursion) {
        push_substring(data.substr(recursion_off), recursion_idx, eof);
    }

    // deal with eof
    if (eof) {
        m_eofPos = index + data.size();
    }

    // write to bytestream
    fflush();
}

size_t StreamReassembler::unassembled_bytes() const {
    return m_substrSize;
}

bool StreamReassembler::empty() const { 
    return m_substrs.empty();
}

void StreamReassembler::fflush() {
    auto it = m_substrs.begin();
    while (it != m_substrs.end()) {
        if (it->first != m_pos) return;

        uint64_t nextIdx = it->first + it->second.size();
        const string& data = it->second;
        size_t sz = data.size();
        auto ret = _output.write(data);
        m_substrSize -= ret;

        // write part of data
        if (ret == 0)
            return;
        if (ret != sz) {
            string &&leftStr = data.substr(sz - ret);
            m_pos = m_pos + ret;
            m_substrs[m_pos] = std::move(leftStr);
            m_substrs.erase(it);
            return;
        }

        m_pos = nextIdx;
        m_substrs.erase(it++);
    }

    if (m_pos == m_eofPos) {
        _output.end_input();
    }
}

#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    _cap = capacity;
    _remain_cap = capacity;
    _byte_stream.resize(capacity);
}

size_t ByteStream::write(const string &data) {
    size_t len = data.size() > _remain_cap ? _remain_cap : data.size();
    for (size_t i = 0; i < len; i++) {
        _byte_stream[_idx_e] = data[i];
        _idx_e = _idx_e + 1 == _cap ? 0 : _idx_e + 1;
    }

    // update record
    _remain_cap -= len;
    _total_written += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    string tmp(rlen, 0);
    size_t sidx = _idx_s;
    for (size_t i = 0; i < rlen; i++) {
        tmp[i] = _byte_stream[sidx];
        sidx = sidx + 1 == _cap ? 0 : sidx + 1;
    }
    return tmp;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    if (len > 0 && rlen == 0) {
        return;
    }
    if (_idx_s + rlen >= _cap) {
        _idx_s = rlen - (_cap - _idx_s);
    } else {
        _idx_s += rlen;
    }
    _remain_cap += rlen;
    _total_read += rlen;
    _eof = buffer_empty() && input_ended();
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    if (len > 0 && rlen == 0) {
        return "";
    }
    string tmp(rlen, 0);
    for (size_t i = 0; i < rlen; i++) {
        tmp[i] = _byte_stream[_idx_s];
        _idx_s = _idx_s + 1 == _cap ? 0 : _idx_s + 1;
    }
    _remain_cap += rlen;
    _total_read += rlen;
    _eof = buffer_empty() && input_ended();
    return tmp;
}

void ByteStream::end_input() {
    _close = true;
    if (buffer_empty()) {
        _eof = true;
    }
}

bool ByteStream::input_ended() const { 
    return _close;
}

size_t ByteStream::buffer_size() const { 
    return _cap - _remain_cap;
}

bool ByteStream::buffer_empty() const { 
    return _cap == _remain_cap;
}

bool ByteStream::eof() const { 
    return _eof;
}

size_t ByteStream::bytes_written() const { 
    return _total_written;
}

size_t ByteStream::bytes_read() const { 
    return _total_read;
}

size_t ByteStream::remaining_capacity() const { 
    return _remain_cap;
}

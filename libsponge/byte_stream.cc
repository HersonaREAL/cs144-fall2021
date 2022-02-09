#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    m_cap = capacity;
    m_remain_cap = capacity;
}

size_t ByteStream::write(const string &data) {
    if (m_close) return 0;
    size_t len = data.size() > m_remain_cap ? m_remain_cap : data.size();
    
    m_bytes.insert(m_bytes.end(), data.begin(), data.begin() + len);

    // update record
    m_remain_cap -= len;
    m_total_written += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    string tmp(m_bytes.begin(), m_bytes.begin() + rlen);
    return tmp;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    m_bytes.erase(m_bytes.begin(), m_bytes.begin() + rlen);
    m_remain_cap += rlen;
    m_total_read += rlen;
    m_eof = buffer_empty() && input_ended();
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t rlen = len > buffer_size() ? buffer_size() : len;
    if (len > 0 && rlen == 0) {
        return "";
    }

    string tmp(m_bytes.begin(), m_bytes.begin() + rlen);
    m_bytes.erase(m_bytes.begin(), m_bytes.begin() + rlen);

    m_remain_cap += rlen;
    m_total_read += rlen;
    m_eof = buffer_empty() && input_ended();
    return tmp;
}

void ByteStream::end_input() {
    m_close = true;
    if (buffer_empty()) {
        m_eof = true;
    }
}

bool ByteStream::input_ended() const { 
    return m_close;
}

size_t ByteStream::buffer_size() const { 
    return m_cap - m_remain_cap;
}

bool ByteStream::buffer_empty() const { 
    return m_cap == m_remain_cap;
}

bool ByteStream::eof() const { 
    return m_eof;
}

size_t ByteStream::bytes_written() const { 
    return m_total_written;
}

size_t ByteStream::bytes_read() const { 
    return m_total_read;
}

size_t ByteStream::remaining_capacity() const { 
    return m_remain_cap;
}

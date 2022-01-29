#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <random>
#include <algorithm>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , m_timer(std::bind(&TCPSender::resend, this))
{
    m_timer.setRto(_initial_retransmission_timeout);
}

uint64_t TCPSender::bytes_in_flight() const { 
    return m_flyBytes;
}

void TCPSender::fill_window(bool fromSingleByte) {
    // judge end
    if (fromSingleByte)
        return;
    if (_stream.bytes_written() + 2 == _next_seqno) return;

    // cal window_size
    size_t winlen = m_rightEdge >= _next_seqno ? m_rightEdge - _next_seqno : 0;
    size_t strlen = std::min(winlen, TCPConfig::MAX_PAYLOAD_SIZE);
    bool singleByte = false;
    bool byteChance = false;
    if (strlen == 0) {
        if (m_flyBytes != 0) return;
        strlen = 1;
        singleByte = true;
    }
    strlen -= _next_seqno == 0;

    // read data
    string &&data = _stream.read(strlen);
    if (data.size() == 0) {
        if (winlen == 0 && !_stream.eof()) return;
        if (_next_seqno != 0 && !_stream.eof() ) return;
        // chance for fin and syn when winSize == 0
        byteChance = true;
    }

    // create segment
    TCPSegment seg;
    seg.header().syn = _next_seqno == 0;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().fin = _stream.eof() && (winlen > data.size() || byteChance);
    seg.payload() = std::move(data);
    
    // send and record
    _segments_out.push(seg);
    m_outstandings.push({_next_seqno, seg});

    // start timer
    if (m_timer.is_stop()) {
        m_timer.start();
    }

    // deal with flag
    size_t seg_len = seg.length_in_sequence_space();
    _next_seqno += seg_len;
    m_flyBytes += seg_len;

    return fill_window(singleByte);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t Aackno = unwrap(ackno, _isn, _next_seqno); // checkpoint ????
    m_rightEdge = Aackno + window_size;
    if (Aackno > _next_seqno) {
        return;
    }

    bool ack_success = false;
    while (!m_outstandings.empty()) {
        if (m_outstandings.front().first >= Aackno) break;
        size_t seg_len = m_outstandings.front().second.length_in_sequence_space();
        if (m_outstandings.front().first + seg_len > Aackno) {
            break;
        }
        ack_success = true;
        m_flyBytes -= seg_len;
        m_outstandings.pop();
    }

    // timer
    if (ack_success) {
        m_timer.setRto(_initial_retransmission_timeout);
        if (!m_outstandings.empty()) m_timer.restart();
        else m_timer.stop();
        m_timer.reset_resent_cnt();
    }


}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    m_timer.inc_elapse(ms_since_last_tick);
}

unsigned int TCPSender::consecutive_retransmissions() const { return m_timer.getRsendCnt(); }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::resend() {
    if (m_outstandings.empty()) {
        m_timer.stop();
        return;
    }

    uint64_t next_rto = m_timer.getRto();
    _segments_out.push(m_outstandings.front().second);
    if (_next_seqno < m_rightEdge || 
            (_next_seqno == m_rightEdge && m_flyBytes != 0)) {
        m_timer.inc_resent_cnt();
        next_rto *= 2;
        m_timer.setRto(next_rto);
    }
    
    m_timer.restart();
}

void TCP_RTTimer::inc_elapse(size_t ms) {
    if (m_stop) return;
    m_elapse += ms;
    if (m_elapse >= m_rto && m_cb) {
        m_cb();
    }
}

void TCP_RTTimer::restart() {
    m_elapse = 0;
    m_stop = false;
}

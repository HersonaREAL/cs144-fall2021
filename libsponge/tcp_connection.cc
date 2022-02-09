#include "tcp_connection.hh"

#include <bits/stdint-uintn.h>
#include <iostream>
#include <stdexcept>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return m_elapse - m_lastRecTime; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // ???
    if (!active()) {
        return;
    }
    
    m_lastRecTime = m_elapse;

    if (seg.header().rst) {
        // close connection
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        
        // TODO kill connection
        m_active = false;
    }

    // give seg to receiver
    _receiver.segment_received(seg);

    // for sender ACK
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    _sender.fill_window();

    // if received an seg which occupied space, send reply
    if (seg.length_in_sequence_space() > 0 && _segments_out.empty() && _sender.segments_out().empty()) {
        // TCPSegment reply_seg;
        // reply_seg.header().seqno = _sender.next_seqno();
        // reply_seg.header().ack = true;
        // if (!_receiver.ackno().has_value())
        //     throw "receiver invaild!";
        // reply_seg.header().ackno = _receiver.ackno().value();
        // reply_seg.header().win = _receiver.window_size();
        // _segments_out.push(reply_seg);
        _sender.send_empty_segment();
    }

    // deal with keep alive seg
    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0)
            && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    // set linger state
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    ready_segments();
}

bool TCPConnection::active() const { return m_active; }

size_t TCPConnection::write(const string &data) {
    size_t ret = _sender.stream_in().write(data);
    _sender.fill_window();
    ready_segments();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    m_elapse += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    ready_segments();

    // TODO end connection cleanly
    if (m_lingering) {
        // std::cout << "timeout time: " << (m_lingerStartTime + 10* (_cfg.rt_timeout))
        //             << " elapse: " << m_elapse << std::endl;
        if ((m_lingerStartTime + 10* (_cfg.rt_timeout)) <= m_elapse) {
            m_active = false;
        }
        return;
    }

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        // send rst
        m_active = false;
        return send_rst_segment();
    }

    
}

void TCPConnection::send_rst_segment() {
    _sender.send_empty_segment();
    _sender.segments_out().back().header().rst = true;
    ready_segments();
}

void TCPConnection::ready_segments() {
    while (!_sender.segments_out().empty()) {
        auto &&seg = _sender.segments_out().front();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = _receiver.window_size();
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }

    // outbound close and inbound close
    if (_sender.stream_in().eof() && !_linger_after_streams_finish) {
        m_active = false;
    } else if (_sender.stream_in().eof() &&
            _receiver.stream_out().eof() && 
            _linger_after_streams_finish && !m_lingering) {
        m_lingering = true;
        m_lingerStartTime = m_elapse;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    ready_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    if (_sender.segments_out().empty() || !_sender.segments_out().front().header().syn)
        throw std::logic_error("SYN send error");
    _segments_out.push(_sender.segments_out().front());
    _sender.segments_out().pop();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

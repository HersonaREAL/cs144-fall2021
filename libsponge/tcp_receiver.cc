#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <initializer_list>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    bool eof = false;
    bool firstInit = false;
    // judge seqno
    if (!m_synSet) {
        if (!seg.header().syn) return;
        m_synSet = true;
        firstInit = true;
        m_ISN = seg.header().seqno;
        m_ackno = m_ISN + 1;
        m_Aackno = 1;
    }

    uint64_t seg_index = unwrap(seg.header().seqno, m_ISN, m_Aackno - 1);
    if (seg_index == 0 && seg.payload().size() > 0 && firstInit) {
        seg_index++;
    }

    // judge fin
    if (!m_finSet && seg.header().fin) {
        // TODO
        m_finSet = true;
        // _reassembler.stream_out().end_input();
        eof = true;

        if (seg_index == 0 && firstInit) {
            //just ISN and FIN
            _reassembler.push_substring("", 0, true);
            m_Aackno = _reassembler.stream_idx() + 1;
            m_ackno = wrap(m_Aackno, m_ISN);
            _reassembler.stream_out().end_input();
        }
    }

    if (seg_index > 0) {
        _reassembler.push_substring(seg.payload().copy(), seg_index - 1, eof);

        m_Aackno = _reassembler.stream_idx() + 1;
        m_ackno = wrap(m_Aackno, m_ISN);
    }
    

    if (_reassembler.isArrivedEof()) {
        m_Aackno++;
        m_ackno = m_ackno + 1;
        _reassembler.stream_out().end_input();
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!m_synSet) return {};
    return {WrappingInt32(m_ackno)};
}

size_t TCPReceiver::window_size() const { 
    return _capacity - _reassembler.stream_out().buffer_size();
}

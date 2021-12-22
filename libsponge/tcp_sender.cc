#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t bytes_outgoing=0;
    for (auto s = _segments_outgoing.cbegin(); s < _segments_outgoing.cend(); s++) {
        bytes_outgoing += s->length_in_sequence_space();
    }
    return bytes_outgoing;
}

void TCPSender::fill_window() {
    if (_window_size == 0) {
        TCPSegment seg{};
        seg.header().seqno = next_seqno();
        _segments_out.push(seg);
        return;
    }
    while (_window_size > 0){
        size_t b_r = stream_in().bytes_read();
        bool syn = b_r == 0 ? true: false;
        bool fin = stream_in().eof();
        TCPSegment seg{};
        seg.header().syn = syn;
        seg.header().fin = fin;
        seg.header().seqno = next_seqno();
        size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE,_window_size);
        seg.payload() = stream_in().read(size);
        _segments_out.push(seg);
        _next_seqno +=seg.length_in_sequence_space();
        _window_size -=size;
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    _next_seqno =
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}

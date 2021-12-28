#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

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
    , _stream(capacity)
    , _rto(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {
    if (_segments_outgoing.empty())
        return 0;
    return wrap(_next_seqno, _isn)  - _segments_outgoing.front().header().seqno;
}

void TCPSender::fill_window() {
    if (!_syn) {
        _syn = true;
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        return;
    }
    if (!_segments_outgoing.empty() && _segments_outgoing.front().header().syn)
        return;
    if (!_stream.buffer_size() && !_stream.eof())
        return;
    if (_fin)
        return;
    if (_receiver_window_zero && _receiver_window_size) {
        TCPSegment seg;
        if (_stream.eof()) {
            _fin = true;
            seg.header().fin = true;
        } else if (!_stream.buffer_empty()) {
            seg.payload() = _stream.read(1);
        }
        send_segment(seg);
    }
    while (_receiver_window_size) {
        TCPSegment seg;
        size_t payload_size = min({_stream.buffer_size(),
                                   static_cast<size_t>(_receiver_window_size),
                                   static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE)});
        seg.payload() = _stream.read(payload_size);
        if (_stream.eof() && static_cast<size_t>(_receiver_window_size) > payload_size) {
            seg.header().fin = true;
            _fin = true;
        }
        send_segment(seg);
        if (_stream.buffer_empty()) {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    uint64_t abs_seqno = unwrap(_segments_outgoing.front().header().seqno, _isn, _next_seqno);
    if (abs_ackno > _next_seqno || abs_ackno < abs_seqno) {
        return;
    }
    _receiver_window_zero = window_size ? false: true;
    _receiver_window_size = _receiver_window_zero ? 1:window_size;
    while (!_segments_outgoing.empty()){
        TCPSegment seg = _segments_outgoing.front();
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno){
            _segments_outgoing.pop();
            _time_elapsed = 0;
            _rto = _initial_retransmission_timeout;
            _consecutive_retransmissions = 0;
        }else {
            break;
        }
    }
    if (!_segments_outgoing.empty()) {
        _receiver_window_size = static_cast<uint16_t>(abs_ackno + static_cast<uint64_t>(window_size)
                                                      - unwrap(_segments_outgoing.front().header().seqno, _isn, _next_seqno)
                                                      - bytes_in_flight());
    }
    if (!bytes_in_flight()) {
        _timer_running= false;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_running)
        return;
    _time_elapsed += ms_since_last_tick;
    if (_time_elapsed >= _rto) {
        _segments_out.push(_segments_outgoing.front());
        //! 窗口大小为 0 时不需要增加 RTO。但是发送 SYN 时，窗口为初始值也为 0，而 SYN 超时是需要增加 RTO 的。
        if (!_receiver_window_zero || _segments_outgoing.front().header().syn) {
            ++_consecutive_retransmissions;
            _rto <<=1;
        }
        _time_elapsed=0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg){
    seg.header().seqno = wrap(_next_seqno, _isn);
    size_t seg_length = seg.length_in_sequence_space();
    _next_seqno += seg_length;
    _receiver_window_size -= seg_length;
    _segments_out.push(seg);
    _segments_outgoing.push(seg);
    if (!_timer_running) {
        _timer_running = true;
        _time_elapsed = 0;
    }
}
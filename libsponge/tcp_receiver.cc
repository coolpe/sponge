#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    uint64_t stream_index = 0;
    //syn flag set
    if (header.syn){
        _syn = true;
        _isn = WrappingInt32(header.seqno);
        if (_syn && header.fin){_fin = true;}
        _reassembler.push_substring(seg.payload().copy(), stream_index, _fin);
        return;
    } else{
        if (!_syn) {return;}
        if (_syn && header.fin){_fin = true;}
        uint64_t checkpoint = stream_out().bytes_written();
        stream_index = unwrap(header.seqno, _isn, checkpoint) - 1;
        _reassembler.push_substring(seg.payload().copy(), stream_index, _fin);
    }

}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) {
        return nullopt;
    }
    uint64_t abs_seqno = stream_out().bytes_written();
    // FIN also accounts for one byte, but make sure that the reassembler
    // have successfully received the EOF signal and ended the input stream.
    // Otherwise, something went wrong and The FIN means nothing.
    if (_fin && stream_out().input_ended()) {
        abs_seqno++;
    }
    return optional<WrappingInt32>(wrap(abs_seqno + 1, _isn));
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }

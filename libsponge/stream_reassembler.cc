#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _buf(capacity, '\0'), _bitmap(capacity, false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _head_index + _output.remaining_capacity()) {
        return;
    }
    if ((index + data.size()) < _head_index){
        if (eof){_eof = true;}
    } else{
        if (index + data.size() <= _head_index + _output.remaining_capacity()) {
            if (eof) {
                _eof = true;
            }
        }

        for (size_t i = index; i < _head_index + _output.remaining_capacity() && i < index + data.size(); i++) {
            if (i >= _head_index && !_bitmap[i - _head_index]) {
                _buf[i - _head_index] = data[i - index];
                _bitmap[i - _head_index] = true;
                _unassembled_bytes++;
            }
        }

        string str = "";
        while (_bitmap.front()) {
            str += _buf.front();
            _buf.pop_front();
            _buf.push_back('\0');
            _bitmap.pop_front();
            _bitmap.push_back(false);
        }

        size_t len = str.size();
        if (len > 0) {
            _unassembled_bytes -= len;
            _head_index += len;
            _output.write(str);
        }
    }

    if (_eof && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

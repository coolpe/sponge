#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_size = data.size();
    if (_capacity < data_size){
        return;
    }
    _capacity -=data_size;
    if (eof) {
        _eof_index = index;
    }
    if (_next_index == index || _output.remaining_capacity() >= data_size) {
        _output.write(data);
        _next_index++;
    } else{
        if (!_una_substring[index]){
            _una_substring[index] = &data;
            _una_bytes += data_size;
        }
    }
    while (_una_substring[_next_index]){
        if (_output.remaining_capacity() < _una_substring[_next_index]->size()) {
            break;
        }
        _output.write(*_una_substring[_next_index]);
        _una_substring[_next_index] = NULL;
        _next_index++;
        _una_bytes -=data_size;
    }

}

size_t StreamReassembler::unassembled_bytes() const { return _una_bytes; }

bool StreamReassembler::empty() const { return _una_bytes == 0; }

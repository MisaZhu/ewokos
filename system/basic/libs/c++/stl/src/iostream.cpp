#include <iostream>
#include <string>

namespace std {

// Standard stream objects
basic_ostream<char> cout;
basic_ostream<char> cerr;
basic_ostream<char> clog;
basic_istream<char> cin;

// Implementation of string output operator
template<>
basic_ostream<char>& basic_ostream<char>::operator<<(const string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        putchar(s[i]);
    }
    return *this;
}

} // namespace std

#pragma once
#include <cstdint>

template <size_t N>
struct XorStr {
    char m_data[N];
    constexpr XorStr(const char (&str)[N]) : m_data{} {
        for (size_t i = 0; i < N; ++i) m_data[i] = str[i] ^ 0x5A;
    }
    const char* get() const {
        static char buf[N];
        for (size_t i = 0; i < N; ++i) buf[i] = m_data[i] ^ 0x5A;
        buf[N-1] = '\0';
        return buf;
    }
    operator const char*() const { return get(); }
};

#define X(s) XorStr(s).get()

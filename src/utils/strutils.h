#pragma once

#include <algorithm> // std::transform
#include <string>

namespace strutils {
// trim from start (in place)
inline void ltrim(std::string &s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}
// trim from end (in place)
inline void rtrim_comments(std::string &s, const std::string &comment_tokens)
{
    size_t pos = s.find_first_of(comment_tokens);
    if (pos != s.npos)
        s.erase(pos);
}
inline void toupper(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

inline bool strcontains(const std::string &s, const std::string &sub)
{
    return s.find(sub) != std::string::npos;
}

inline bool streq(const std::string &s1, const std::string &s2)
{
    return s1 == s2;
}

inline bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}
};

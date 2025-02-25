//
// Created by qiyu on 12/19/17.
//

#ifndef CINATRA_UTILS_HPP
#define CINATRA_UTILS_HPP

#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>  //std::byte
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#include "define.h"
#include "sha1.hpp"

namespace cinatra {
struct ci_less {
  // case-independent (ci) compare_less binary function
  struct nocase_compare {
    bool operator()(const unsigned char &c1, const unsigned char &c2) const {
      return tolower(c1) < tolower(c2);
    }
  };

  bool operator()(const std::string &s1, const std::string &s2) const {
    return std::lexicographical_compare(s1.begin(), s1.end(),  // source range
                                        s2.begin(), s2.end(),  // dest range
                                        nocase_compare());     // comparison
  }

  bool operator()(std::string_view s1, std::string_view s2) const {
    return std::lexicographical_compare(s1.begin(), s1.end(),  // source range
                                        s2.begin(), s2.end(),  // dest range
                                        nocase_compare());     // comparison
  }
};

class noncopyable {
 public:
  noncopyable() = default;
  ~noncopyable() = default;

 private:
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
};

using namespace std::string_view_literals;

template <class T>
struct sv_char_trait : std::char_traits<T> {
  using base_t = std::char_traits<T>;
  using char_type = typename base_t::char_type;

  static constexpr int compare(std::string_view s1,
                               std::string_view s2) noexcept {
    if (s1.length() != s2.length())
      return -1;

    size_t n = s1.length();
    for (size_t i = 0; i < n; ++i) {
      if (!base_t::eq(s1[i], s2[i])) {
        return base_t::eq(s1[i], s2[i]) ? -1 : 1;
      }
    }

    return 0;
  }

  static constexpr size_t find(std::string_view str,
                               const char_type &a) noexcept {
    auto s = str.data();
    for (size_t i = 0; i < str.length(); ++i) {
      if (base_t::eq(s[i], a)) {
        return i;
      }
    }

    return std::string_view::npos;
  }
};

inline std::string_view trim_left(std::string_view v) {
  v.remove_prefix((std::min)(v.find_first_not_of(" "), v.size()));
  return v;
}

inline std::string_view trim_right(std::string_view v) {
  v.remove_suffix((std::min)(v.size() - v.find_last_not_of(" ") - 1, v.size()));
  return v;
}

inline std::string_view trim(std::string_view v) {
  v.remove_prefix((std::min)(v.find_first_not_of(" "), v.size()));
  v.remove_suffix((std::min)(v.size() - v.find_last_not_of(" ") - 1, v.size()));
  return v;
}

inline std::pair<std::string_view, std::string_view> get_domain_url(
    std::string_view path) {
  size_t size = path.size();
  size_t pos = std::string_view::npos;
  for (size_t i = 0; i < size; i++) {
    if (path[i] == '/') {
      if (i == size - 1) {
        pos = i;
        break;
      }

      if (i + 1 < size - 1 && path[i + 1] == '/') {
        i++;
        continue;
      }
      else {
        pos = i;
        break;
      }
    }
  }

  if (pos == std::string_view::npos) {
    return {path, "/"};
  }

  std::string_view host = path.substr(0, pos);
  std::string_view url = path.substr(pos);
  if (url.length() > 1 && url.back() == '/') {
    url = url.substr(0, url.length() - 1);
  }

  return {host, url};
}
inline std::string_view remove_www(std::string_view path) {
  if (path.back() == '/') {
    path = std::string_view(path.data(), path.length() - 1);
  }
  if (path.find("www.") != std::string_view::npos)
    return path.substr(4);

  return path;
}

inline std::pair<std::string, std::string> get_host_port(std::string_view path,
                                                         bool is_ssl) {
  std::string_view old_path = path;
  size_t pos = path.rfind(':');
  if (pos == std::string_view::npos) {
    if (path.find("https") != std::string_view::npos) {
      return {std::string(remove_www(path)), "https"};
    }

    return {std::string(remove_www(path)), is_ssl ? "https" : "http"};
  }

  if (pos > path.length() - 1) {
    return {};
  }

  if (path.find("http") != std::string_view::npos) {
    size_t pos1 = path.find(':');
    if (pos1 + 3 > path.length() - 1)
      return {};

    path = path.substr(pos1 + 3);
    if (pos >= (pos1 + 3))
      pos -= (pos1 + 3);
  }

  if (old_path[pos - 1] == 'p') {
    return {std::string(remove_www(path)), "http"};
  }
  else if (old_path[pos - 1] == 's') {
    return {std::string(remove_www(path)), "https"};
  }

  return {std::string(path.substr(0, pos)), std::string(path.substr(pos + 1))};
}

inline void SHA1(uint8_t *key_src, size_t size, uint8_t *sha1buf) {
  sha1_context ctx;
  init(ctx);
  update(ctx, key_src, size);
  finish(ctx, sha1buf);
}

template <typename T>
constexpr bool is_int64_v =
    std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t>;

enum class http_method {
  UNKNOW,
  DEL,
  GET,
  HEAD,
  POST,
  PUT,
  PATCH,
  CONNECT,
  OPTIONS,
  TRACE
};
constexpr inline auto GET = http_method::GET;
constexpr inline auto POST = http_method::POST;
constexpr inline auto DEL = http_method::DEL;
constexpr inline auto HEAD = http_method::HEAD;
constexpr inline auto PUT = http_method::PUT;
constexpr inline auto CONNECT = http_method::CONNECT;
#ifdef TRACE
#undef TRACE
constexpr inline auto TRACE = http_method::TRACE;
#endif
constexpr inline auto OPTIONS = http_method::OPTIONS;

enum class transfer_type { CHUNKED, ACCEPT_RANGES };

inline constexpr std::string_view method_name(http_method mthd) {
  switch (mthd) {
    case cinatra::http_method::DEL:
      return "DELETE"sv;
      break;
    case cinatra::http_method::GET:
      return "GET"sv;
      break;
    case cinatra::http_method::HEAD:
      return "HEAD"sv;
      break;
    case cinatra::http_method::POST:
      return "POST"sv;
      break;
    case cinatra::http_method::PUT:
      return "PUT"sv;
      break;
    case cinatra::http_method::PATCH:
      return "PATCH"sv;
      break;
    case cinatra::http_method::CONNECT:
      return "CONNECT"sv;
      break;
    case cinatra::http_method::OPTIONS:
      return "OPTIONS"sv;
      break;
    case cinatra::http_method::TRACE:
      return "TRACE"sv;
      break;
    default:
      return "UNKONWN"sv;
      break;
  }
}

inline std::string get_content_type_str(req_content_type type) {
  std::string str;
  switch (type) {
    case req_content_type::html:
      str = "text/html; charset=UTF-8";
      break;
    case req_content_type::json:
      str = "application/json; charset=UTF-8";
      break;
    case req_content_type::string:
      str = "text/html; charset=UTF-8";
      break;
    case req_content_type::multipart:
      str = "multipart/form-data; boundary=";
      break;
    case req_content_type::none:
    default:
      break;
  }

  return str;
}

constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::DEL>) noexcept {
  return "DELETE"sv;
}
constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::GET>) noexcept {
  return "GET"sv;
}
constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::HEAD>) noexcept {
  return "HEAD"sv;
}

constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::POST>) noexcept {
  return "POST"sv;
}
constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::PUT>) noexcept {
  return "PUT"sv;
}

constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::CONNECT>) noexcept {
  return "CONNECT"sv;
}
constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::OPTIONS>) noexcept {
  return "OPTIONS"sv;
}
constexpr auto type_to_name(
    std::integral_constant<http_method, http_method::TRACE>) noexcept {
  return "TRACE"sv;
}

inline bool iequal(const char *s, size_t l, const char *t) {
  if (strlen(t) != l)
    return false;

  for (size_t i = 0; i < l; i++) {
    if (std::tolower(s[i]) != std::tolower(t[i]))
      return false;
  }

  return true;
}

inline bool iequal(const char *s, size_t l, const char *t, size_t size) {
  if (size != l)
    return false;

  for (size_t i = 0; i < l; i++) {
    if (std::tolower(s[i]) != std::tolower(t[i]))
      return false;
  }

  return true;
}

template <typename T>
inline bool find_strIC(const T &src, const T &dest) {
  auto it = std::search(src.begin(), src.end(), dest.begin(), dest.end(),
                        [](char ch1, char ch2) {
                          return std::toupper(ch1) == std::toupper(ch2);
                        });
  return (it != src.end());
}

inline std::vector<std::string_view> split(std::string_view s,
                                           std::string_view delimiter) {
  size_t start = 0;
  size_t end = s.find_first_of(delimiter);

  std::vector<std::string_view> output;

  while (end <= std::string_view::npos) {
    output.emplace_back(s.substr(start, end - start));

    if (end == std::string_view::npos)
      break;

    start = end + 1;
    end = s.find_first_of(delimiter, start);
  }

  return output;
}

inline void remove_char(std::string &str, const char ch) {
  str.erase(std::remove(str.begin(), str.end(), ch), str.end());
}

template <typename... Args>
inline void print(Args... args) {
  ((std::cout << args << ' '), ...);
  std::cout << "\n";
}

inline void print(const std::error_code &ec) {
  print(ec.value(), ec.message());
}

// var bools = [];
// var valid_chr =
// 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-'; for(var
// i = 0; i <= 127; ++ i) { 	var contain =
// valid_chr.indexOf(String.fromCharCode(i)) == -1;
// 	bools.push(contain?false:true);
// }
// console.log(JSON.stringify(bools))

inline const constexpr bool valid_chr[128] = {
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false,
    false, true,  true,  false, true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  false, false, false, false, false, false, false, true,
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  false, false, false, false, true,  false, true,  true,
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
    true,  true,  false, false, false, false, false};

inline std::ostringstream &quote_impl(std::ostringstream &os,
                                      std::string_view str,
                                      std::string_view safe) {
  os << std::setiosflags(std::ios::right) << std::setfill('0');
  auto begin = reinterpret_cast<const std::byte *>(str.data());
  auto end = begin + sizeof(char) * str.size();
  std::for_each(begin, end, [&os, &safe](auto &chr) {
    char chrval = (char)chr;
    unsigned int intval = (unsigned int)chr;
    if ((intval > 127 || !valid_chr[intval]) &&
        safe.find(chrval) == std::string_view::npos)
      os << '%' << std::setw(2) << std::hex << std::uppercase << intval;
    else
      os << chrval;
  });
  return os;
}

inline const std::string quote(std::string_view str) {
  std::ostringstream os;
  return quote_impl(os, str, "/").str();
}

inline const std::string quote_plus(std::string_view str) {
  if (str.find(' ') == std::string_view::npos)
    return quote(str);

  std::ostringstream os;
  auto strval = quote_impl(os, str, " ").str();
  std::replace(strval.begin(), strval.end(), ' ', '+');
  return strval;
}

inline std::string form_urldecode(const std::string &src) {
  std::string ret;
  char ch;
  int i, ii;
  for (i = 0; i < src.length(); i++) {
    if (int(src[i]) == 37) {
      sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
      ch = static_cast<char>(ii);
      ret += ch;
      i = i + 2;
    }
    else {
      ret += src[i];
    }
  }
  return ret;
}

inline bool is_form_url_encode(std::string_view str) {
  return str.find("%") != std::string_view::npos ||
         str.find("+") != std::string_view::npos;
}

inline std::string_view get_extension(std::string_view name) {
  size_t pos = name.rfind('.');
  if (pos == std::string_view::npos) {
    return {};
  }

  return name.substr(pos);
}

inline bool is_status_ok(int status) {
  return (status == 200) || (status >= 301 && status <= 307 && status != 306);
}

inline std::string to_hex_string(std::size_t value) {
  std::ostringstream stream;
  stream << std::hex << value;
  return stream.str();
}

inline int64_t hex_to_int(std::string_view s) {
  if (s.empty())
    return -1;

  char *p;
  int64_t n = strtoll(s.data(), &p, 16);
  if (n == 0 && s.front() != '0') {
    return -1;
  }

  return n;
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

inline std::string base64_encode(const std::string &str) {
  std::string ret;
  int i = 0;
  int j = 0;
  char char_array_3[3];
  char char_array_4[4];

  auto bytes_to_encode = str.data();
  size_t in_len = str.size();
  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

inline std::string base64_decode(std::string const &encoded_string) {
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

// from h2o
inline const char *MAP =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline const char *MAP_URL_ENCODED =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";
inline size_t base64_encode(char *_dst, const void *_src, size_t len,
                            int url_encoded) {
  char *dst = _dst;
  const uint8_t *src = reinterpret_cast<const uint8_t *>(_src);
  const char *map = url_encoded ? MAP_URL_ENCODED : MAP;
  uint32_t quad;

  for (; len >= 3; src += 3, len -= 3) {
    quad = ((uint32_t)src[0] << 16) | ((uint32_t)src[1] << 8) | src[2];
    *dst++ = map[quad >> 18];
    *dst++ = map[(quad >> 12) & 63];
    *dst++ = map[(quad >> 6) & 63];
    *dst++ = map[quad & 63];
  }
  if (len != 0) {
    quad = (uint32_t)src[0] << 16;
    *dst++ = map[quad >> 18];
    if (len == 2) {
      quad |= (uint32_t)src[1] << 8;
      *dst++ = map[(quad >> 12) & 63];
      *dst++ = map[(quad >> 6) & 63];
      if (!url_encoded)
        *dst++ = '=';
    }
    else {
      *dst++ = map[(quad >> 12) & 63];
      if (!url_encoded) {
        *dst++ = '=';
        *dst++ = '=';
      }
    }
  }

  *dst = '\0';
  return dst - _dst;
}

inline bool is_valid_utf8(unsigned char *s, size_t length) {
  for (unsigned char *e = s + length; s != e;) {
    if (s + 4 <= e && ((*(uint32_t *)s) & 0x80808080) == 0) {
      s += 4;
    }
    else {
      while (!(*s & 0x80)) {
        if (++s == e) {
          return true;
        }
      }

      if ((s[0] & 0x60) == 0x40) {
        if (s + 1 >= e || (s[1] & 0xc0) != 0x80 || (s[0] & 0xfe) == 0xc0) {
          return false;
        }
        s += 2;
      }
      else if ((s[0] & 0xf0) == 0xe0) {
        if (s + 2 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
            (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||
            (s[0] == 0xed && (s[1] & 0xe0) == 0xa0)) {
          return false;
        }
        s += 3;
      }
      else if ((s[0] & 0xf8) == 0xf0) {
        if (s + 3 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
            (s[3] & 0xc0) != 0x80 || (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||
            (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) {
          return false;
        }
        s += 4;
      }
      else {
        return false;
      }
    }
  }
  return true;
}

template <typename T>
inline std::string to_str(T &&value) {
  using U = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (std::is_integral_v<U> && !is_int64_v<U>) {
    std::vector<char> temp(20, '\0');
    itoa_fwd(value, temp.data());
    return std::string(temp.data());
  }
  else if constexpr (is_int64_v<U>) {
    std::vector<char> temp(65, '\0');
    xtoa(value, temp.data(), 10, std::is_signed_v<U>);
    return std::string(temp.data());
  }
  else if constexpr (std::is_floating_point_v<U>) {
    std::vector<char> temp(20, '\0');
    sprintf(temp.data(), "%f", value);
    return std::string(temp.data());
  }
  else if constexpr (std::is_same_v<std::string, U> ||
                     std::is_same_v<const char *, U>) {
    return value;
  }
  else {
    std::cout << "this type has not supported yet" << std::endl;
  }
}

// for is_detective
namespace {
struct nonesuch {
  nonesuch() = delete;
  ~nonesuch() = delete;
  nonesuch(const nonesuch &) = delete;
  void operator=(const nonesuch &) = delete;
};

template <class Default, class AlwaysVoid, template <class...> class Op,
          class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  using value_t = std::true_type;
  using type = Op<Args...>;
};

template <template <class...> class Op, class... Args>
using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

template <class T, typename... Args>
using has_before_t =
    decltype(std::declval<T>().before(std::declval<Args>()...));

template <class T, typename... Args>
using has_after_t = decltype(std::declval<T>().after(std::declval<Args>()...));
}  // namespace

template <typename T, typename... Args>
using has_before = is_detected<has_before_t, T, Args...>;

template <typename T, typename... Args>
using has_after = is_detected<has_after_t, T, Args...>;

template <typename... Args, typename F, std::size_t... Idx>
constexpr void for_each_l(std::tuple<Args...> &t, F &&f,
                          std::index_sequence<Idx...>) {
  (std::forward<F>(f)(std::get<Idx>(t)), ...);
}

template <typename... Args, typename F, std::size_t... Idx>
constexpr void for_each_r(std::tuple<Args...> &t, F &&f,
                          std::index_sequence<Idx...>) {
  constexpr auto size = sizeof...(Idx);
  (std::forward<F>(f)(std::get<size - Idx - 1>(t)), ...);
}

template <http_method N>
constexpr void get_str(std::string &s, std::string_view name) {
  s = type_to_name(std::integral_constant<http_method, N>{}).data();
  s += std::string(name.data(), name.length());
}

template <http_method... Is>
constexpr auto get_arr(std::string_view name) {
  std::array<std::string, sizeof...(Is)> arr = {};
  size_t index = 0;
  (get_str<Is>(arr[index++], name), ...);

  return arr;
}

template <http_method... Is>
constexpr auto get_method_arr() {
  std::array<char, 26> arr{0};
  std::string_view s;
  ((s = type_to_name(std::integral_constant<http_method, Is>{}),
    arr[s[0] - 65] = s[0]),
   ...);

  return arr;
}

inline std::string get_time_str(std::time_t t) {
  std::stringstream ss;
  ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

inline std::string get_gmt_time_str(std::time_t t) {
  struct tm *GMTime = gmtime(&t);
  char buff[512] = {0};
  strftime(buff, sizeof(buff), "%a, %d %b %Y %H:%M:%S %Z", GMTime);
  return buff;
}

namespace time_util {
inline constexpr bool is_leap(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline constexpr int get_day_index(std::string_view str) {
  return week_table[((str[0] & ~0x20) ^ (str[2] & ~0x20)) % week_table.size()];
}

inline constexpr int get_month_index(std::string_view str) {
  return month_table[((str[1] & ~0x20) + (str[2] & ~0x20)) %
                     month_table.size()];
}

inline constexpr int days_in(int m, int year) {
  constexpr int index = get_month_index("Feb");
  if (m == index && is_leap(year)) {
    return 29;
  }
  return int(days_before[m + 1] - days_before[m]);
}

inline constexpr int get_digit(std::string_view sv, int width) {
  int num = 0;
  for (int i = 0; i < width; i++) {
    if ('0' <= sv[i] && sv[i] <= '9') {
      num = num * 10 + (sv[i] - '0');
    }
    else {
      return -1;
    }
  }
  return num;
}

inline constexpr std::uint64_t days_since_epoch(int year) {
  auto y = std::uint64_t(std::int64_t(year) - absolute_zero_year);

  auto n = y / 400;
  y -= 400 * n;
  auto d = days_per_400_years * n;
  n = y / 100;
  y -= 100 * n;
  d += days_per_100_years * n;
  n = y / 4;
  y -= 4 * n;
  d += days_per_4_years * n;
  n = y;
  d += 365 * n;
  return d;
}

inline std::pair<bool, std::time_t> faster_mktime(int year, int month, int day,
                                                  int hour, int min, int sec,
                                                  int day_of_week) {
  auto d = days_since_epoch(year);
  d += std::uint64_t(days_before[month]);
  constexpr int index = get_month_index("Mar");
  if (is_leap(year) && month >= index) {
    d++;  // February 29
  }
  d += std::uint64_t(day - 1);
  auto abs = d * seconds_per_day;
  abs +=
      std::uint64_t(hour * seconds_per_hour + min * seconds_per_minute + sec);
  constexpr int day_index = get_day_index("Mon");
  if (day_of_week != -1) {
    std::int64_t wday = ((abs + std::uint64_t(day_index) * seconds_per_day) %
                         seconds_per_week) /
                        seconds_per_day;
    if (wday != day_of_week) {
      return {false, 0};
    }
  }
  return {true, std::int64_t(abs) + (absolute_to_internal + internal_to_unix)};
}

template <time_format Format>
inline constexpr std::array<component_of_time_format, 32> get_format() {
  if constexpr (Format == time_format::http_format) {
    return http_time_format;
  }
  else if constexpr (Format == time_format::utc_format) {
    return utc_time_format;
  }
  else {
    return utc_time_without_punctuation_format;
  }
}
}  // namespace time_util

template <time_format Format = time_format::http_format>
inline std::pair<bool, std::time_t> get_timestamp(
    const std::string &gmt_time_str) {
  using namespace time_util;
  std::string_view sv(gmt_time_str);
  int year, month, day, hour, min, sec, day_of_week;
  int len_of_gmt_time_str = (int)gmt_time_str.length();
  int len_of_processed_part = 0;
  int len_of_ignored_part = 0;  // second_decimal_part is ignored
  char c;
  constexpr std::array<component_of_time_format, 32> real_format =
      time_util::get_format<Format>();
  if constexpr (Format == time_format::utc_format) {
    day_of_week = -1;
  }
  else if (Format == time_format::utc_without_punctuation_format) {
    day_of_week = -1;
  }

  for (auto &comp : real_format) {
    switch (comp) {
      case component_of_time_format::ending:
        goto travel_done;
        break;
      case component_of_time_format::colon:
      case component_of_time_format::comma:
      case component_of_time_format::SP:
      case component_of_time_format::hyphen:
      case component_of_time_format::dot:
      case component_of_time_format::T:
      case component_of_time_format::Z:
        if (len_of_gmt_time_str - len_of_processed_part < 1) {
          return {false, 0};
        }
        c = sv[len_of_processed_part];
        if ((comp == component_of_time_format::Z && c != 'Z') ||
            (comp == component_of_time_format::T && c != 'T') ||
            (comp == component_of_time_format::dot && c != '.') ||
            (comp == component_of_time_format::hyphen && c != '-') ||
            (comp == component_of_time_format::SP && c != ' ') ||
            (comp == component_of_time_format::colon && c != ':') ||
            (comp == component_of_time_format::comma && c != ',')) {
          return {false, 0};
        }
        len_of_processed_part += 1;
        break;
      case component_of_time_format::year:
        if (len_of_gmt_time_str - len_of_processed_part < 4) {
          return {false, 0};
        }
        if ((year = get_digit(sv.substr(len_of_processed_part, 4), 4)) == -1) {
          return {false, 0};
        }
        len_of_processed_part += 4;
        break;
      case component_of_time_format::month_name:
        if (len_of_gmt_time_str - len_of_processed_part < 3) {
          return {false, 0};
        }
        if ((month = get_month_index(sv.substr(len_of_processed_part, 3))) ==
            -1) {
          return {false, 0};
        }
        len_of_processed_part += 3;
        break;
      case component_of_time_format::hour:
      case component_of_time_format::minute:
      case component_of_time_format::second:
      case component_of_time_format::month:
      case component_of_time_format::day:
        if (len_of_gmt_time_str - len_of_processed_part < 2) {
          return {false, 0};
        }
        int digit;
        if ((digit = get_digit(sv.substr(len_of_processed_part, 2), 2)) == -1) {
          return {false, 0};
        }
        if (comp == component_of_time_format::hour) {
          hour = digit;
          if (hour < 0 || hour >= 24) {
            return {false, 0};
          }
        }
        else if (comp == component_of_time_format::minute) {
          min = digit;
          if (min < 0 || min >= 60) {
            return {false, 0};
          }
        }
        else if (comp == component_of_time_format::month) {
          month = digit;
          if (month < 1 || month > 12) {
            return {false, 0};
          }
          month--;
        }
        else if (comp == component_of_time_format::second) {
          sec = digit;
          if (sec < 0 || sec >= 60) {
            return {false, 0};
          }
        }
        else {
          day = digit;
        }
        len_of_processed_part += 2;
        break;
      case component_of_time_format::day_name:
        if (len_of_gmt_time_str - len_of_processed_part < 3) {
          return {false, 0};
        }
        if ((day_of_week = get_day_index(sv.substr(len_of_processed_part, 3))) <
            0) {
          return {false, 0};
        }
        len_of_processed_part += 3;
        break;
      case component_of_time_format::GMT:
        if (len_of_gmt_time_str - len_of_processed_part < 3) {
          return {false, 0};
        }
        if (sv.substr(len_of_processed_part, 3) != "GMT") {
          return {false, 0};
        }
        len_of_processed_part += 3;
        break;
      case component_of_time_format::second_decimal_part:
        int cur = len_of_processed_part;
        while (cur < len_of_gmt_time_str &&
               (sv[cur] >= '0' && sv[cur] <= '9')) {
          len_of_ignored_part++;
          cur++;
        }
        if (cur == len_of_processed_part) {
          return {false, 0};
        }
        len_of_processed_part = cur;
        break;
    }
  }
travel_done:
  if ((len_of_processed_part != len_of_gmt_time_str) ||
      (len_of_processed_part != len_of_http_time_format &&
       (len_of_processed_part - len_of_ignored_part) !=
           len_of_utc_time_format) &&
          (len_of_processed_part - len_of_ignored_part) !=
              len_of_utc_time_without_punctuation_format) {
    return {false, 0};
  }
  if (day < 1 || day > days_in(month, year)) {
    return {false, 0};
  }
  return faster_mktime(year, month, day, hour, min, sec, day_of_week);
}

inline std::string get_cur_time_str() {
  return get_time_str(std::time(nullptr));
}

inline const std::map<std::string_view, std::string_view> get_cookies_map(
    std::string_view cookies_str) {
  std::map<std::string_view, std::string_view> cookies;
  auto cookies_vec = split(cookies_str, "; ");
  for (auto iter : cookies_vec) {
    auto cookie_key_vlaue = split(iter, "=");
    if (cookie_key_vlaue.size() == 2) {
      cookies[cookie_key_vlaue[0]] = cookie_key_vlaue[1];
    }
  }
  return cookies;
};

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>>
    : std::disjunction<std::is_same<T, Us>...> {};

template <typename T>
struct filter_helper {
  static constexpr auto func() { return std::tuple<>(); }

  template <class... Args>
  static constexpr auto func(T &&, Args &&...args) {
    return filter_helper::func(std::forward<Args>(args)...);
  }

  template <class X, class... Args>
  static constexpr auto func(X &&x, Args &&...args) {
    return std::tuple_cat(std::make_tuple(std::forward<X>(x)),
                          filter_helper::func(std::forward<Args>(args)...));
  }
};

template <typename T, typename... Args>
inline auto filter(Args &&...args) {
  return filter_helper<T>::func(std::forward<Args>(args)...);
}
}  // namespace cinatra

#endif  // CINATRA_UTILS_HPP

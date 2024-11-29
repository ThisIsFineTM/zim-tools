/*
 * Copyright 2013-2016 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright 2016 Matthieu Gautier <mgautier@kymeria.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <zim-tools/tools.h>

#include <algorithm>   // for any_of, count, find, min, mismatch, transform
#include <array>       // for array
#include <cctype>      // for isxdigit, isalnum, tolower
#include <cstdio>      // for sscanf
#include <cstring>     // for strncmp
#include <filesystem>  // for path, exists, is_directory, operator/
#include <iomanip>     // for operator<<, setw
#include <ios>
#include <iterator>  // for back_insert_iterator, back_inserter, next
#include <map>       // for map, operator==
#include <optional>  // for optional, nullopt, nullopt_t
#include <sstream>   // for basic_ostringstream, basic_ostream, basic_ist...
#include <string>
#include <string_view>  // for string_view, basic_string_view, operator<=>
#include <utility>
#include <vector>  // for vector

// #ifdef _WIN32
////constexpr std::string_view SEPARATOR{R"(\\)"};
// #else
// #include <unistd.h>
////constexpr std::string_view SEPARATOR{R"(/)"};
// #endif

bool fileExists(std::string_view path)
{
  return std::filesystem::exists(std::filesystem::path{path});
}

bool isDirectory(std::string_view path)
{
  return std::filesystem::is_directory(std::filesystem::path{path});
}

// base64
constexpr static std::string_view base64_chars
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len)
{
  std::string ret;
  ret.reserve(in_len);

  size_t i = 0;
  size_t j = 0;

  std::array<unsigned char, 3> char_array_3{};
  std::array<unsigned char, 4> char_array_4{};

  while ((in_len--) != 0u) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1]
          = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2]
          = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) {
        ret += base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }

  if (i != 0u) {
    for (j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1]
        = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2]
        = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) {
      ret += base64_chars[char_array_4[j]];
    }

    while ((i++ < 3)) {
      ret += '=';
    }
  }

  return ret;
}

static char charFromHex(const std::string& a)
{
  std::istringstream Blat(a);
  int Z = 0;
  Blat >> std::hex >> Z;
  return static_cast<char>(Z);
}

std::string decodeUrl(std::string_view originalUrl)
{
  std::string url{originalUrl};
  std::string::size_type pos = 0;

  while ((pos = url.find('%', pos)) != std::string::npos
         && pos + 2 < url.length()) {
    if ((std::isxdigit(url[pos + 1]) == 0)
        || (std::isxdigit(url[pos + 2]) == 0)) {
      ++pos;
      continue;
    }
    url.replace(pos, 3, 1, charFromHex(url.substr(pos + 1, 2)));
    ++pos;
  }
  return url;
}

namespace
{

// TODO: test trailing delim
std::vector<std::string> split(std::string_view str, char delim)
{
  std::vector<std::string> result;
  const auto* start = str.cbegin();
  while (true) {
    const auto* const end = std::find(start, str.cend(), delim);

    result.emplace_back(start, end);
    if (end == str.cend()) {
      break;
    }
    start = end + 1;
  }

  return result;
}

std::string getRelativePath(std::string_view basePath,
                            std::string_view targetPath)
{
  const auto b = split(basePath, '/');
  const auto t = split(targetPath, '/');
  const size_t l = std::min(b.size() - 1, t.size());

  auto x = std::mismatch(b.cbegin(), b.cbegin() + l, t.cbegin());

  const size_t ups = (b.cend() - x.first) + (x.second == t.cend());
  //const size_t ups
  //    = (b.cend() - x.first)
  //      + static_cast<typename __normal_iterator<
  //          const class basic_string<char>*,
  //          class vector<class basic_string<char>>>::difference_type>(
  //          x.second == t.cend());

  std::string r;

  for (size_t i = 1; i < ups; ++i) {
    if (!r.empty() && r.back() != '/') {
      r += "/";
    }
    r += "..";
  }

  for (auto it = x.second; it != t.end(); ++it) {
    if (!r.empty() && r.back() != '/') {
      r += "/";
    }
    r += *it;
  }
  return r;
}

}  // unnamed namespace

std::string computeRelativePath(std::string_view basePath,
                                std::string_view targetPath)
{
  if (targetPath.back() == '/') {
    if (basePath == targetPath) {
      return std::string{"./"};
    }

    std::string_view const strippedPath{
        targetPath.substr(0, targetPath.size() - 1)};

    return getRelativePath(basePath, strippedPath) + '/';
  }
  return getRelativePath(basePath, targetPath);
}

/* Warning: the relative path must be with slashes */
std::string computeAbsolutePath(std::string_view path,
                                std::string_view relativePath)
{
  std::filesystem::path fsPath{path};

  if (!path.empty() && path.back() != '/') {
    fsPath = fsPath.parent_path();
  }

  auto absolutePath = fsPath / relativePath;

  return absolutePath.string();
}

void replaceStringInPlaceOnce(std::string& subject,
                              std::string_view search,
                              std::string_view replace)
{
  const size_t pos = subject.find(search, 0);
  if (pos != std::string::npos) {
    subject.replace(pos, search.length(), replace);
  }
}

void replaceStringInPlace(std::string& subject,
                          std::string_view search,
                          std::string_view replace)
{
  if (search.empty()) {
    return;
  }

  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

void stripTitleInvalidChars(std::string& str)
{
  /* Remove unicode orientation invisible characters */
  replaceStringInPlace(str, "\u202A", "");
  replaceStringInPlace(str, "\u202C", "");
}

namespace
{

std::optional<const std::string_view> getHtmlEntity(const std::string_view core)
{
  static const std::map<const std::string_view, const std::string_view> t = {
      {"amp", "&"},
      {"apos", "'"},
      {"quot", "\""},
      {"lt", "<"},
      {"gt", ">"},
  };

  using Return_t = std::optional<const std::string_view>;

  const auto it = t.find(core);
  return it != t.end() ? Return_t{std::in_place, it->second} : std::nullopt;
}

template <typename Itr>
std::string_view make_sv(Itr start, Itr end)
{
  return std::string_view(start, std::distance(start, end));
}

}  // unnamed namespace

std::string decodeHtmlEntities(std::string_view str)
{
  std::string result;

  const auto* p = str.cbegin();
  const auto* start = str.cend();

  for (; p != str.cend(); ++p) {
    if (*p == '&') {
      if (start != str.cend()) {
        result.append(start, p);
      }
      start = p;
    } else if (start == str.cend()) {
      result.push_back(*p);
    } else if (*p == ';') {
      if (auto d = getHtmlEntity(make_sv(start + 1, p))) {
        result += d.value();
      } else {
        result.append(start, p + 1);
      }

      start = str.cend();
    }
  }

  if (start != str.cend()) {
    result.append(start, p);
  }

  return result;
}

std::vector<html_link> generic_getLinks(std::string_view page)
{
  const char* p = page.data();
  const char* linkStart{};
  std::vector<html_link> links;
  std::string attr;

  while (*p != 0) {
    if (std::strncmp(p, " href", 5) == 0) {
      attr = "href";
      p += 5;
    } else if (std::strncmp(p, " src", 4) == 0) {
      attr = "src";
      p += 4;
    } else {
      p += 1;
      continue;
    }

    while (*p == ' ') {
      p += 1;
    }
    if (*(p++) != '=') {
      continue;
    }
    while (*p == ' ') {
      p += 1;
    }
    char const delimiter = *p++;
    if (delimiter != '\'' && delimiter != '"') {
      continue;
    }

    linkStart = p;

    // [TODO] Handle escape char
    while (*p != delimiter) {
      p++;
    }
    links.emplace_back(attr, decodeHtmlEntities(make_sv(linkStart, p)));
    p += 1;
  }
  return links;
}

bool isOutofBounds(std::string_view input, std::string base)
{
  if (input.empty()) {
    return false;
  }

  if (base.empty() || base.back() != '/') {
    base.push_back('/');
  }

  int nr = 0;
  if (base.front() != '/') {
    nr++;
  }

  // count nr of substrings ../
  int nrsteps = 0;
  std::string_view::size_type pos = 0;
  while ((pos = input.find("../", pos)) != std::string::npos) {
    nrsteps++;
    pos += 3;
  }

  return nrsteps >= (nr + std::count(base.cbegin(), base.cend(), '/'));
}

unsigned int adler32(std::string_view buf) noexcept
{
  constexpr unsigned int adler32magic{65521};

  unsigned int s1 = 1;
  unsigned int s2 = 0;

  for (auto c : buf) {
    s1 = (s1 + c) % adler32magic;
    s2 = (s2 + s1) % adler32magic;
  }
  return (s2 << 16) | s1;
}

std::string normalize_link(const std::string_view input,
                           const std::string_view baseUrl)
{
  std::string output;
  output.reserve(baseUrl.size() + input.size() + 1);

  bool check_rel = false;
  // const char* p = input.c_str();
  const auto* p = input.cbegin();
  if (*(p) == '/') {
    // This is an absolute url.
    p++;
  } else {
    // This is a relative url, use base url
    output = baseUrl;
    if (!output.empty() && output.back() != '/') {
      output += '/';
    }
    check_rel = true;
  }

  constexpr static std::string_view parentPrefix{"../"};
  constexpr static std::string_view currentPrefix{"./"};

  // URL Decoding.
  while (p < input.cend()) {
    if (check_rel) {
      if (std::strncmp(p, parentPrefix.data(), parentPrefix.size()) == 0) {
        // We must go "up"
        // Remove the '/' at the end of output.
        output.resize(output.size() - 1);

        // Remove the last part.
        const size_t pos = output.find_last_of('/');
        output.resize(pos == std::string::npos ? 0 : pos);

        // Move after the "..".
        p += 2;
        check_rel = false;
        continue;
      }
      if (std::strncmp(p, currentPrefix.data(), currentPrefix.size()) == 0) {
        // We must simply skip this part
        // Simply move after the ".".
        p += 2;
        check_rel = false;
        continue;
      }
    }

    if (*p == '#' || *p == '?') {
      // For our purposes we can safely discard the query and/or fragment
      // components of the URL
      break;
    }

    if (*p == '%') {
      unsigned char ch = 0;
      sscanf(p + 1, "%2hhx", &ch);  // hhx only supports hex unsigned char
      output += ch;
      p += 3;
      continue;
    }

    if (*p == '/') {
      check_rel = true;
      if (output.empty()) {
        // Do not add '/' at beginning of output
        p++;
        continue;
      }
    }
    output += *(p++);
  }
  return output;
}

namespace
{

UriKind specialUriSchemeKind(const std::string_view s)
{
  static const std::map<const std::string_view, UriKind> uriSchemes
      = {{"javascript", UriKind::JAVASCRIPT},
         {"mailto", UriKind::MAILTO},
         {"tel", UriKind::TEL},
         {"sip", UriKind::SIP},
         {"geo", UriKind::GEO},
         {"data", UriKind::DATA},
         {"xmpp", UriKind::XMPP},
         {"news", UriKind::NEWS},
         {"urn", UriKind::URN}};

  const auto it = uriSchemes.find(s);
  return it != uriSchemes.cend() ? it->second : UriKind::OTHER;
}

constexpr std::string_view uriDelimiters{":/?#"};
constexpr std::string_view uriProtocolRelativeDelimeter{"//"};

}  // unnamed namespace

UriKind html_link::detectUriKind(std::string_view input_string) noexcept
{
  const size_t k = input_string.find_first_of(uriDelimiters);

  if (k == std::string_view::npos || input_string[k] != ':') {
    if (k == 0 && input_string.size() > 1
        && input_string.substr(0, 2) == uriProtocolRelativeDelimeter) {
      return UriKind::PROTOCOL_RELATIVE;
    }
    return UriKind::OTHER;
  }

  if (k + 2 < input_string.size() && input_string[k + 1] == '/'
      && input_string[k + 2] == '/') {
    return UriKind::GENERIC_URI;
  }

  std::string scheme;
  scheme.reserve(k);

  std::transform(input_string.cbegin(),
                 std::next(input_string.cbegin(), k),
                 std::back_inserter(scheme),
                 [](auto c) { return std::tolower(c); });

  return specialUriSchemeKind(scheme);
}

namespace
{

bool isReservedUrlChar(char c) noexcept
{
  constexpr static std::array<char, 10> reserved
      = {{';', ',', '?', ':', '@', '&', '=', '+', '$'}};

  return std::any_of(
      reserved.cbegin(), reserved.cend(), [c](char elem) { return elem == c; });
}

bool needsEscape(const char c, const bool encodeReserved)
{
  if (std::isalnum(c) != 0) {
    return false;
  }

  if (isReservedUrlChar(c)) {
    return encodeReserved;
  }

  constexpr std::array<char, 10> noNeedEscape
      = {{'-', '_', '.', '!', '~', '*', '\'', '(', ')', '/'}};

  return not std::any_of(noNeedEscape.begin(),
                         noNeedEscape.end(),
                         [c](char elem) { return elem == c; });
}

std::string urlEncode(std::string_view value, bool encodeReserved)
{
  std::ostringstream os;
  os << std::hex << std::uppercase;

  for (auto c : value) {
    if (!needsEscape(c, encodeReserved)) {
      os << c;
    } else {
      os << '%' << std::setw(2)
         << static_cast<unsigned int>(static_cast<unsigned char>(c));
    }
  }
  return os.str();
}

}  // unnamed namespace

std::string httpRedirectHtml(std::string_view redirectUrl)
{
  const auto encodedurl = urlEncode(redirectUrl, true);
  std::ostringstream ss;

  ss << "<!DOCTYPE html>"
          "<html>"
          "<head>"
          "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
          "<meta http-equiv=\"refresh\" content=\"0;url=" + encodedurl + "\" />"
          "</head>"
          "<body></body>"
          "</html>";
  return ss.str();
}

bool guess_is_front_article(std::string_view mimetype)
{
  constexpr static std::string_view mimeTextHtml{"text/html"};
  constexpr static std::string_view mimeRawTrue{"raw=true"};

  return (mimetype.find(mimeTextHtml) == 0
          && mimetype.find(mimeRawTrue) == std::string_view::npos);
}

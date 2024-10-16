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

#ifndef OPENZIM_TOOLS_H
#define OPENZIM_TOOLS_H

#include <zim/item.h>
#include <zim/writer/contentProvider.h>
#include <zim/writer/item.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

/* Formatter for std::exception what() message:
 * throw std::runtime_error(
 *   Formatter() << "zimwriterfs: Unable to read" << filename << ": " <<
 * strerror(errno));
 */
class Formatter final
{
 public:
  Formatter() = default;
  ~Formatter() = default;

  Formatter(const Formatter&) = delete;
  Formatter(Formatter&&) noexcept = default;

  Formatter& operator=(Formatter&) = delete;
  Formatter& operator=(Formatter&&) noexcept = default;

  template <typename Type>
  Formatter& operator<<(Type&& value)
  {
    stream_ << std::forward<Type>(value);
    return *this;
  }

  [[nodiscard]] operator std::string() const { return stream_.str(); }

 private:
  std::stringstream stream_;
};

enum class UriKind : int {
  // Special URIs without authority that are valid in HTML
  JAVASCRIPT,  // javascript:...
  MAILTO,      // mailto:user@example.com
  TEL,         // tel:+0123456789
  SIP,         // sip:1-999-123-4567@voip-provider.example.net
  GEO,         // geo:12.34,56.78
  DATA,        // data:image/png;base64,...
  XMPP,        // xmpp:kelson@kiwix.org
  NEWS,        // news:comp.os.linux.announce
  URN,         // urn:nbn:de:bsz:24-digibib-bsz3530416370

  GENERIC_URI,        // Generic URI with scheme and authority: <scheme>://.....
  PROTOCOL_RELATIVE,  // Protocol-relative URL: //<host>/<path>/<to>/<resource>

  OTHER  // not a valid URI (though it can be a valid relative
         // or absolute URL)
};

class html_link final
{
 public:
  html_link() = delete;
  ~html_link() = default;

  html_link(const std::string_view _attr, const std::string_view _link)
      : attribute_(_attr), link_(_link), uriKind_(detectUriKind(_link))
  {
  }

  html_link(const html_link&) = default;
  html_link(html_link&&) noexcept = default;

  html_link& operator=(const html_link&) = default;
  html_link& operator=(html_link&&) noexcept = default;

  [[nodiscard]]
  constexpr bool isExternalUrl() const noexcept
  {
    return uriKind_ != UriKind::OTHER && uriKind_ != UriKind::DATA;
  }

  [[nodiscard]]
  constexpr bool isInternalUrl() const noexcept
  {
    return uriKind_ == UriKind::OTHER;
  }

  [[nodiscard]]
  static UriKind detectUriKind(const std::string_view input_string) noexcept;

  [[nodiscard]]
  constexpr const std::string& attribute() const noexcept
  {
    return attribute_;
  }

  [[nodiscard]]
  constexpr const std::string& link() const noexcept
  {
    return link_;
  }

  [[nodiscard]]
  constexpr UriKind uriKind() const noexcept
  {
    return uriKind_;
  }

 private:
  std::string attribute_;
  std::string link_;
  UriKind uriKind_;
};

// Few helper class to help copy a item from a archive to another one.
class ItemProvider : public zim::writer::ContentProvider
{
 private:
  zim::Item item;
  bool feeded;

 public:
  ItemProvider(zim::Item item) : item(item), feeded(false) {}

  zim::size_type getSize() const { return item.getSize(); }

  zim::Blob feed()
  {
    if (feeded) {
      return zim::Blob();
    }
    feeded = true;
    return item.getData();
  }
};

// Guess if the item is a front article.
// This is not a exact science, we use the mimetype to infer it.
bool guess_is_front_article(std::string_view mimetype);

class CopyItem
    : public zim::writer::Item  // Article class that will be passed to the
                                // zimwriter. Contains a zim::Article class, so
                                // it is easier to add a
{
  // article from an existing ZIM file.
  zim::Item item;

 public:
  explicit CopyItem(const zim::Item item) : item(item) {}

  virtual std::string getPath() const { return item.getPath(); }

  virtual std::string getTitle() const { return item.getTitle(); }

  virtual std::string getMimeType() const { return item.getMimetype(); }

  std::unique_ptr<zim::writer::ContentProvider> getContentProvider() const
  {
    return std::unique_ptr<zim::writer::ContentProvider>(
        new ItemProvider(item));
  }

  zim::writer::Hints getHints() const
  {
    return {{zim::writer::HintKeys::FRONT_ARTICLE,
             guess_is_front_article(item.getMimetype())}};
  }
};

std::string getMimeTypeForFile(const std::string& basedir,
                               const std::string& filename);
std::string getFileContent(const std::string& path);
std::string decodeUrl(std::string_view encodedUrl);

// Assuming that basePath and targetPath are relative to the same location
// returns the relative path of targetPath from basePath
std::string computeRelativePath(std::string_view basePath,
                                std::string_view targetPath);

std::string computeAbsolutePath(std::string_view path,
                                std::string_view relativePath);

[[nodiscard]]
bool fileExists(const std::string_view path);

[[nodiscard]]
bool isDirectory(std::string_view path);

std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len);

void replaceStringInPlaceOnce(std::string& subject,
                              const std::string& search,
                              const std::string& replace);
void replaceStringInPlace(std::string& subject,
                          const std::string& search,
                          const std::string& replace);
void stripTitleInvalidChars(std::string& str);

// Returns a vector of the links in a particular page. includes links under
// 'href' and 'src'
std::vector<html_link> generic_getLinks(std::string_view page);

// checks if a relative path is out of bounds (relative to base)
bool isOutofBounds(const std::string& input, std::string base);

// Adler32 Hash Function. Used to hash the BLOB data obtained from each article,
// for redundancy checks. Please note that the adler32 hash function has a high
// number of collisions, and that the hash match is not taken as final.
// Adler32 is only used in an index elsewhere. Changing to unsigned.
unsigned int adler32(std::string_view buf) noexcept;

std::string decodeHtmlEntities(std::string_view str);

// Removes extra spaces from URLs. Usually done by the browser, so web authors
// sometimes tend to ignore it. Converts the %20 to space.Essential for
// comparing URLs.
std::string normalize_link(std::string_view input, std::string_view baseUrl);

std::string httpRedirectHtml(std::string_view redirectUrl);
#endif  //  OPENZIM_TOOLS_H

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

#include <zim/blob.h>                    // for Blob
#include <zim/item.h>                    // for Item
#include <zim/writer/contentProvider.h>  // for ContentProvider
#include <zim/writer/item.h>             // for HintKeys, Item, Hints
#include <zim/zim.h>                     // for size_type

#include <cstddef>      // for size_t
#include <memory>       // for unique_ptr, make_unique
#include <sstream>      // for basic_stringstream, char_traits
#include <string>       // for basic_string, string
#include <string_view>  // for string_view
#include <utility>      // for move, forward, pair
#include <vector>       // for vector
#include <utility>

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

  html_link(std::string_view attr, std::string_view link)
      : attribute_(attr), link_(link), uriKind_(detectUriKind(link))
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
  static UriKind detectUriKind(std::string_view input_string) noexcept;

  [[nodiscard]]
  std::string_view attribute() const noexcept
  {
    return attribute_;
  }

  [[nodiscard]]
  std::string_view link() const noexcept
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
  using Parent_t = zim::writer::ContentProvider;

  zim::Item item_;
  bool feeded_{};

 public:
  ItemProvider() = delete;
  ~ItemProvider() override = default;

  ItemProvider(const ItemProvider&) = default;
  ItemProvider(ItemProvider&&) noexcept = default;

  ItemProvider(zim::Item item) noexcept
      : Parent_t(), item_(std::move(item)), feeded_(false)
  {
  }

  ItemProvider& operator=(const ItemProvider&) = default;
  ItemProvider& operator=(ItemProvider&&) noexcept = default;

  [[nodiscard]]
  zim::size_type getSize() const override
  {
    return item_.getSize();
  }

  [[nodiscard]]
  zim::Blob feed() override
  {
    if (feeded_) {
      return zim::Blob();
    }
    feeded_ = true;
    return item_.getData();
  }
};

// Guess if the item is a front article.
// This is not an exact science, so we use the mimetype to infer it.
[[nodiscard]]
bool guess_is_front_article(std::string_view mimetype);

class CopyItem
    : public zim::writer::Item  // Article class that will be passed to the
                                // zimwriter. Contains a zim::Article class, so
                                // it is easier to add a
{
  using Parent_t = zim::writer::Item;

  // article from an existing ZIM file.
  zim::Item item_;

 public:
  CopyItem() = delete;
  ~CopyItem() override = default;

  CopyItem(const CopyItem&) = default;
  CopyItem(CopyItem&&) noexcept = default;
  explicit CopyItem(const zim::Item& item) : Parent_t(), item_(item) {}

  CopyItem& operator=(const CopyItem&) = default;
  CopyItem& operator=(CopyItem&&) noexcept = default;

  [[nodiscard]]
  std::string getPath() const override
  {
    return item_.getPath();
  }

  [[nodiscard]]
  std::string getTitle() const override
  {
    return item_.getTitle();
  }

  [[nodiscard]]
  std::string getMimeType() const override
  {
    return item_.getMimetype();
  }

  [[nodiscard]]
  std::unique_ptr<zim::writer::ContentProvider> getContentProvider()
      const override
  {
    return std::make_unique<ItemProvider>(item_);
  }

  [[nodiscard]]
  zim::writer::Hints getHints() const override
  {
    return {{zim::writer::HintKeys::FRONT_ARTICLE,
             guess_is_front_article(item_.getMimetype())}};
  }
};

[[nodiscard]]
std::string decodeUrl(std::string_view encodedUrl);

// Assuming that basePath and targetPath are relative to the same location
// returns the relative path of targetPath from basePath
[[nodiscard]]
std::string computeRelativePath(std::string_view basePath,
                                std::string_view targetPath);

[[nodiscard]]
std::string computeAbsolutePath(std::string_view path,
                                std::string_view relativePath);

[[nodiscard]]
bool fileExists(std::string_view path);

[[nodiscard]]
bool isDirectory(std::string_view path);

[[nodiscard]]
std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len);

void replaceStringInPlaceOnce(std::string& subject,
                              std::string_view search,
                              std::string_view replace);

void replaceStringInPlace(std::string& subject,
                          std::string_view search,
                          std::string_view replace);

void stripTitleInvalidChars(std::string& str);

// Returns a vector of the links in a particular page. includes links under
// 'href' and 'src'
[[nodiscard]]
std::vector<html_link> generic_getLinks(std::string_view page);

// checks if a relative path is out of bounds (relative to base)
[[nodiscard]]
bool isOutofBounds(std::string_view input, std::string base);

// Adler32 Hash Function. Used to hash the BLOB data obtained from each article,
// for redundancy checks. Please note that the adler32 hash function has a high
// number of collisions, and that the hash match is not taken as final.
// Adler32 is only used in an index elsewhere. Changing to unsigned.
[[nodiscard]]
unsigned int adler32(std::string_view buf) noexcept;

[[nodiscard]]
std::string decodeHtmlEntities(std::string_view str);

// Removes extra spaces from URLs. Usually done by the browser, so web authors
// sometimes tend to ignore it. Converts the %20 to space.Essential for
// comparing URLs.
[[nodiscard]]
std::string normalize_link(std::string_view input, std::string_view baseUrl);

[[nodiscard]]
std::string httpRedirectHtml(std::string_view redirectUrl);

template <typename T, typename... Args>
constexpr bool is_any_of(T&& lhs, Args&&... args) noexcept
{
    return ((std::forward<T>(lhs) == std::forward<Args>(args)) || ...);
}

#endif  //  OPENZIM_TOOLS_H

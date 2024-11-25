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

#include <zim-tools/zimwriterfs/tools.h>
//
#include <gumbo.h>     // for GumboNode, GumboAttribute, GumboInternalNode
//
#ifndef ZLIB_CONST
#define ZLIB_CONST
#endif
#include <zlib.h>  // for Z_OK, inflate, inflateEnd, inflateInit_, Z_ST...
//
#include <errno.h>  // for errno
#include <magic.h>  // for magic_file, magic_t
//
#include <algorithm>    // for search
#include <cctype>       // for tolower
#include <cstring>      // for memset, strncmp
#include <ctime>        // for tm, localtime, time, time_t
#include <filesystem>   // for path, operator/
#include <fstream>      // for basic_ostream, basic_ifstream, operator<<
#include <iomanip>      // for operator<<, setfill, setw
#include <iostream>     // for cerr
#include <iterator>     // for next
#include <map>          // for map, operator==, _Rb_tree_const_iterator, _Rb...
#include <sstream>      // for basic_ostringstream, basic_stringstream
#include <stdexcept>    // for runtime_error
#include <string_view>  // for basic_string_view, operator<=>, string_view
#include <utility>      // for pair

constexpr std::string_view mimeTextHtml{"text/html"};
constexpr std::string_view mimeImagePng{"image/png"};
constexpr std::string_view mimeImageTiff{"image/tiff"};
constexpr std::string_view mimeImageJpeg{"image/jpeg"};
constexpr std::string_view mimeImageGif{"image/gif"};
constexpr std::string_view mimeImageSvgXml{"image/svg+xml"};
constexpr std::string_view mimeTextPlain{"text/plain"};
constexpr std::string_view mimeTextXml{"text/xml"};
constexpr std::string_view mimeAppEpubZip{"application/epub+zip"};
constexpr std::string_view mimeAppPdf{"application/pdf"};
constexpr std::string_view mimeAudioOgg{"audio/ogg"};
constexpr std::string_view mimeVideoOgg{"video/ogg"};
constexpr std::string_view mimeAppJavascript{"application/javascript"};
constexpr std::string_view mimeAppJson{"application/json"};
constexpr std::string_view mimeTextCss{"text/css"};
constexpr std::string_view mimeFontOtf{"font/otf"};
constexpr std::string_view mimeFontSfnt{"font/sfnt"};
constexpr std::string_view mimeAppVndMSFontObj{"application/vnd.ms-fontobject"};
constexpr std::string_view mimeFontTtf{"font/ttf"};
constexpr std::string_view mimeFontCollection{"font/collection"};
constexpr std::string_view mimeFontWoff{"font/woff"};
constexpr std::string_view mimeFontWoff2{"font/woff2"};
constexpr std::string_view mimeTextVtt{"text/vtt"};
constexpr std::string_view mimeVideoWebm{"video/webm"};
constexpr std::string_view mimeImageWebp{"image/webp"};
constexpr std::string_view mimeVideoMp4{"video/mp4"};
constexpr std::string_view mimeAppMSWord{"application/msword"};
constexpr std::string_view mimeAppVndXmlWord{
    "application/vnd.openxmlforMats-officedocument.wordprocessingml.document"};
constexpr std::string_view mimeAppVndMSPpt{"application/vnd.ms-powerpoint"};
constexpr std::string_view mimeVndOODoc{
    "application/vnd.oasis.openDocument.text"};
constexpr std::string_view mimeAppZip{"application/zip"};
constexpr std::string_view mimeAppWasm{"application/wasm"};
constexpr std::string_view mimeAppOctetStream{"application/octet-stream"};

/* init file extensions hash */
[[nodiscard]]
static std::map<std::string_view, std::string_view> _create_extMimeTypes()
{
  std::map<std::string_view, std::string_view> extMimeTypes;
  extMimeTypes["HTML"] = mimeTextHtml;
  extMimeTypes["html"] = mimeTextHtml;
  extMimeTypes["HTM"] = mimeTextHtml;
  extMimeTypes["htm"] = mimeTextHtml;
  extMimeTypes["PNG"] = mimeImagePng;
  extMimeTypes["png"] = mimeImagePng;
  extMimeTypes["TIFF"] = mimeImageTiff;
  extMimeTypes["tiff"] = mimeImageTiff;
  extMimeTypes["TIF"] = mimeImageTiff;
  extMimeTypes["tif"] = mimeImageTiff;
  extMimeTypes["JPEG"] = mimeImageJpeg;
  extMimeTypes["jpeg"] = mimeImageJpeg;
  extMimeTypes["JPG"] = mimeImageJpeg;
  extMimeTypes["jpg"] = mimeImageJpeg;
  extMimeTypes["GIF"] = mimeImageGif;
  extMimeTypes["gif"] = mimeImageGif;
  extMimeTypes["SVG"] = mimeImageSvgXml;
  extMimeTypes["svg"] = mimeImageSvgXml;
  extMimeTypes["TXT"] = mimeTextPlain;
  extMimeTypes["txt"] = mimeTextPlain;
  extMimeTypes["XML"] = mimeTextXml;
  extMimeTypes["xml"] = mimeTextXml;
  extMimeTypes["EPUB"] = mimeAppEpubZip;
  extMimeTypes["epub"] = mimeAppEpubZip;
  extMimeTypes["PDF"] = mimeAppPdf;
  extMimeTypes["pdf"] = mimeAppPdf;
  extMimeTypes["OGG"] = mimeAudioOgg;
  extMimeTypes["ogg"] = mimeAudioOgg;
  extMimeTypes["OGV"] = mimeVideoOgg;
  extMimeTypes["ogv"] = mimeVideoOgg;
  extMimeTypes["JS"] = mimeAppJavascript;
  extMimeTypes["js"] = mimeAppJavascript;
  extMimeTypes["JSON"] = mimeAppJson;
  extMimeTypes["json"] = mimeAppJson;
  extMimeTypes["CSS"] = mimeTextCss;
  extMimeTypes["css"] = mimeTextCss;
  extMimeTypes["OTF"] = mimeFontOtf;
  extMimeTypes["otf"] = mimeFontOtf;
  extMimeTypes["SFNT"] = mimeFontSfnt;
  extMimeTypes["sfnt"] = mimeFontSfnt;
  extMimeTypes["EOT"] = mimeAppVndMSFontObj;
  extMimeTypes["eot"] = mimeAppVndMSFontObj;
  extMimeTypes["TTF"] = mimeFontTtf;
  extMimeTypes["ttf"] = mimeFontTtf;
  extMimeTypes["COLLECTION"] = mimeFontCollection;
  extMimeTypes["collection"] = mimeFontCollection;
  extMimeTypes["WOFF"] = mimeFontWoff;
  extMimeTypes["woff"] = mimeFontWoff;
  extMimeTypes["WOFF2"] = mimeFontWoff2;
  extMimeTypes["woff2"] = mimeFontWoff2;
  extMimeTypes["VTT"] = mimeTextVtt;
  extMimeTypes["vtt"] = mimeTextVtt;
  extMimeTypes["WEBM"] = mimeVideoWebm;
  extMimeTypes["webm"] = mimeVideoWebm;
  extMimeTypes["WEBP"] = mimeImageWebp;
  extMimeTypes["webp"] = mimeImageWebp;
  extMimeTypes["MP4"] = mimeVideoMp4;
  extMimeTypes["mp4"] = mimeVideoMp4;
  extMimeTypes["DOC"] = mimeAppMSWord;
  extMimeTypes["doc"] = mimeAppMSWord;
  extMimeTypes["DOCX"] = mimeAppVndXmlWord;
  extMimeTypes["docx"] = mimeAppVndXmlWord;
  extMimeTypes["PPT"] = mimeAppVndMSPpt;
  extMimeTypes["ppt"] = mimeAppVndMSPpt;
  extMimeTypes["ODT"] = mimeVndOODoc;
  extMimeTypes["odt"] = mimeVndOODoc;
  extMimeTypes["ODP"] = mimeVndOODoc;
  extMimeTypes["odp"] = mimeVndOODoc;
  extMimeTypes["ZIP"] = mimeAppZip;
  extMimeTypes["zip"] = mimeAppZip;
  extMimeTypes["WASM"] = mimeAppWasm;
  extMimeTypes["wasm"] = mimeAppWasm;

  return extMimeTypes;
}

const static std::map<std::string_view, std::string_view> extMimeTypes
    = _create_extMimeTypes();

static std::map<std::string, std::string> fileMimeTypes{};

extern bool inflateHtmlFlag;

extern magic_t magic;

// decompress an stl string using zlib and return the original data.
[[nodiscard]]
static std::string inflateString(std::string_view str)
{
  ::z_stream zs;  // z_stream is zlib's control structure
  std::memset(&zs, 0, sizeof(zs));

  if (::inflateInit(&zs) != Z_OK) {
    throw(std::runtime_error("inflateInit failed while decompressing."));
  }

  // bytef is unsigned char (legacy)
  zs.next_in = reinterpret_cast<decltype(zs.next_in)>(str.data());
  zs.avail_in = str.size();

  int ret{};
  char outbuffer[32768];
  std::string outstring;

  // get the decompressed bytes blockwise using repeated calls to inflate
  do {
    zs.next_out = reinterpret_cast<decltype(zs.next_out)>(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = ::inflate(&zs, 0);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  ::inflateEnd(&zs);

  if (ret != Z_STREAM_END) {  // an error occurred that was not EOF
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
    throw(std::runtime_error(oss.str()));
  }

  return outstring;
}

[[nodiscard]]
static bool seemsToBeHtml(std::string_view path)
{
  constexpr static int comparesEqual{0};

  if (path.find_last_of(".") != std::string_view::npos) {
    std::string_view mimetype = path.substr(path.find_last_of(".") + 1);
    if (auto search = extMimeTypes.find(mimetype);
        search != extMimeTypes.end()) {
      return search->second.compare(mimeTextHtml) == comparesEqual;
    }
  }

  return false;
}

std::string getFileContent(std::string_view path)
{
  // ate seeks to the end of the file immediately
  std::ifstream in(std::filesystem::path(path),
                   std::ios::binary | std::ios::ate);
  if (in) {
    std::string contents;
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    /* inflate if necessary */
    if (inflateHtmlFlag && seemsToBeHtml(path)) {
      try {
        contents = inflateString(contents);
      } catch (...) {
        std::cerr << "Can not initialize inflate stream for: " << path
                  << std::endl;
      }
    }
    return contents;
  }
  std::cerr << "zimwriterfs: Unable to open file at path: " << path
            << std::endl;
  throw(errno);
}

std::string extractRedirectUrlFromHtml(const ::GumboVector* head_children)
{
  if (nullptr == head_children) {
    throw std::string(
        "zimwriterfs::tools::extractRedirectUrlFromHtml received nullptr.");
  }

  constexpr static std::string_view attrRefresh{"refresh"};
  constexpr static std::string_view attrHttpEquiv{"http-equiv"};
  constexpr static std::string_view attrContent{"content"};
  constexpr static std::string_view urlPrefix{"url="};
  constexpr static int strcmp_equal{0};

  using gumbo_length_t = decltype(::GumboVector::length);

  std::string url;

  constexpr auto childIsNodeWithMetaTag
      = [](::GumboNode const* const child) constexpr -> bool {
    return child->type == GUMBO_NODE_ELEMENT
           && child->v.element.tag == GUMBO_TAG_META;
  };

  constexpr auto childAttributeIsHttpEquivRefresh
      = [](::GumboNode const* const child) -> bool {
    ::GumboAttribute* attribute = ::gumbo_get_attribute(
        &child->v.element.attributes, attrHttpEquiv.data());

    return nullptr != attribute
           and strcmp_equal
                   == std::strncmp(attribute->value,
                                   attrRefresh.data(),
                                   attrRefresh.size());
  };

  constexpr auto childAttributeIsContent =
      [](::GumboNode const* const child, ::GumboAttribute*& attribute) -> bool {
    attribute = ::gumbo_get_attribute(&child->v.element.attributes,
                                      attrContent.data());

    return nullptr != attribute;
  };

  for (gumbo_length_t i = 0; i < head_children->length; ++i) {
    ::GumboNode* child = static_cast<::GumboNode*>(head_children->data[i]);
    ::GumboAttribute* attribute{nullptr};

    if (nullptr == child  //
        or not childIsNodeWithMetaTag(child)
        or not childAttributeIsHttpEquivRefresh(child)
        or not childAttributeIsContent(child, attribute)) {
      continue;
    }

    std::string_view targetUrl = attribute->value;

    auto foundItr
        = std::search(targetUrl.begin(),
                      targetUrl.end(),
                      urlPrefix.begin(),
                      urlPrefix.end(),
                      [](unsigned char target, unsigned char prefix) {
                        return std::tolower(target) == std::tolower(prefix);
                      });

    if (foundItr != targetUrl.end()) {
      url = std::string(std::next(foundItr, urlPrefix.size()), targetUrl.end());
    } else {
      throw std::string(
          "Unable to find the redirect/refresh target url from the "
          "HTML DOM");
    }
  }

  return url;
}

std::string generateDate()
{
  time_t t = time(0);
  struct tm* now = localtime(&t);
  std::stringstream stream;
  stream << (now->tm_year + 1900) << '-' << std::setw(2) << std::setfill('0')
         << (now->tm_mon + 1) << '-' << std::setw(2) << std::setfill('0')
         << now->tm_mday;
  return stream.str();
}

std::string getMimeTypeForFile(std::string_view directoryPath,
                               std::string_view filename)
{
  namespace fs = std::filesystem;

  fs::path fsPath = fs::path{directoryPath} / filename;

  fs::path extension = fsPath.extension();

  // try to get the mimetype from the file extension
  if (auto mime = extMimeTypes.find(extension.native());
      mime != extMimeTypes.end()) {
    return std::string{mime->second};
  }

  // check the cache
  if (auto mime = fileMimeTypes.find(extension.native());
      mime != fileMimeTypes.cend()) {
    return std::string{mime->second};
  }

  std::string mimeType;

  /* Try to get the mimeType with libmagic */
  try {
    // std::string path = directoryPath + "/" + filename;
    mimeType = std::string(magic_file(magic, fsPath.c_str()));
    if (mimeType.find(";") != std::string::npos) {
      mimeType = mimeType.substr(0, mimeType.find(";"));
    }
    fileMimeTypes[std::string{filename}] = mimeType;
  } catch (...) {
  }

  if (mimeType.empty()) {
    return std::string{mimeAppOctetStream};
  } else {
    return mimeType;
  }
}

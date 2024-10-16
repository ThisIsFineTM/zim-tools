/*
 * Copyright (C) 2006 Tommi Maekitalo
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#define ZIM_PRIVATE
#include <docopt/docopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zim/archive.h>
#include <zim/item.h>

#include <stdexcept>
#include <unordered_set>

#include "tools.h"
#include "version.h"
#ifdef _WIN32
#include <io.h>
#include <windows.h>
constexpr std::string_view SEPARATOR{R"(\\)"};
#else
constexpr std::string_view SEPARATOR{R"(/)"};
#include <unistd.h>
#endif

constexpr std::string_view ERRORSDIR{R"(_exceptions/)"};

#ifdef _WIN32
std::wstring utf8ToUtf16(const std::string& string)
{
  auto size = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, nullptr, 0);
  auto wdata = std::wstring(size, 0);
  auto ret
      = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wdata.data(), size);
  if (0 == ret) {
    std::ostringstream oss;
    oss << "Cannot convert string to wchar : " << GetLastError();
    throw std::runtime_error(oss.str());
  }
  return wdata;
}
#endif

inline static void createdir(std::string_view path, std::string_view base)
{
  namespace fs = std::filesystem;

  fs::path fsPath{path};
  fs::path fsBase{base};
  fs::path fullPath = fsBase / fsPath;

  // do we want to create with 0777 or let the system decide?
  // 0777 might have unintended consequences
  // fs::perms perms = fs::perms::all;
  // filesystem uses all by default

  fs::create_directories(fullPath);

  // std::size_t position = 0;
  // while(position != std::string::npos) {
  //     position = path.find('/', position+1);
  //     if (position != std::string::npos) {
  //         std::string fulldir = base + SEPARATOR + path.substr(0, position);
  //         #if defined(_WIN32)
  //         std::wstring wfulldir = utf8ToUtf16(fulldir);
  //         CreateDirectoryW(wfulldir.c_str(), NULL);
  //         #else
  //           ::mkdir(fulldir.c_str(), 0777);
  //         #endif
  //     }
  // }
}

class ZimDumper
{
  zim::Archive m_archive;
  bool verbose;

 public:
  ZimDumper(const std::string& fname) : m_archive(fname), verbose(false) {}

  void setVerbose(bool sw = true) { verbose = sw; }

  void printInfo();
  int dumpEntry(const zim::Entry& entry);
  int listEntries(bool info);
  int listEntry(const zim::Entry& entry);
  void listEntryT(const zim::Entry& entr);
  int listEntriesByNamespace(const std::string ns, bool details);

  zim::Entry getEntryByPath(const std::string& path);
  zim::Entry getEntryByNsAndPath(char ns, const std::string& path);
  zim::Entry getEntry(zim::size_type idx);

  void dumpFiles(std::string_view directory,
                 bool symlinkdump,
                 std::function<bool(const char c)> nsfilter);

 private:
  void writeHttpRedirect(std::string_view directory,
                         std::string_view relative_path,
                         std::string_view currentEntryPath,
                         std::string_view redirectPath);
};

zim::Entry ZimDumper::getEntryByPath(const std::string& path)
{
  return m_archive.getEntryByPath(path);
}

zim::Entry ZimDumper::getEntryByNsAndPath(char ns, const std::string& path)
{
  return m_archive.getEntryByPathWithNamespace(ns, path);
}

zim::Entry ZimDumper::getEntry(zim::size_type idx)
{
  return m_archive.getEntryByPath(idx);
}

void ZimDumper::printInfo()
{
  std::cout << "count-entries: " << m_archive.getEntryCount() << "\n";
  std::cout << "uuid: " << m_archive.getUuid() << "\n"
            << "cluster count: " << m_archive.getClusterCount() << "\n";
  if (m_archive.hasChecksum()) {
    std::cout << "checksum: " << m_archive.getChecksum() << "\n";
  } else {
    std::cout << "no checksum\n";
  }

  if (m_archive.hasMainEntry()) {
    std::cout << "main page: "
              << m_archive.getMainEntry().getItem(true).getPath() << "\n";
  } else {
    std::cout << "main page: -\n";
  }

  if (m_archive.hasIllustration()) {
    std::cout << "favicon: " << m_archive.getIllustrationItem().getPath()
              << "\n";
  } else {
    std::cout << "favicon: -\n";
  }
  std::cout << std::endl;

  std::cout.flush();
}

int ZimDumper::dumpEntry(const zim::Entry& entry)
{
  if (entry.isRedirect()) {
    std::cerr << "Entry " << entry.getPath() << " is a redirect." << std::endl;
    return -1;
  }

  std::cout << entry.getItem().getData() << std::flush;
  return 0;
}

int ZimDumper::listEntries(bool info)
{
  int ret = 0;
  for (auto& entry : m_archive.iterByPath()) {
    if (info) {
      ret = listEntry(entry);
    } else {
      std::cout << entry.getPath() << '\n';
    }
  }
  return ret;
}

int ZimDumper::listEntry(const zim::Entry& entry)
{
  std::cout << "path: " << entry.getPath()
            << "\n"
               "* title:          "
            << entry.getTitle()
            << "\n"
               "* idx:            "
            << entry.getIndex()
            << "\n"
               "* type:           "
            << (entry.isRedirect() ? "redirect" : "item") << "\n";

  if (entry.isRedirect()) {
    std::cout << "* redirect index: " << entry.getRedirectEntry().getIndex()
              << "\n";
  } else {
    auto item = entry.getItem();
    std::cout << "* mime-type:      " << item.getMimetype()
              << "\n"
                 "* item size:      "
              << item.getSize() << "\n";
  }

  return 0;
}

void ZimDumper::listEntryT(const zim::Entry& entry)
{
  std::cout << entry.getPath() << '\t' << entry.getTitle() << '\t'
            << entry.getIndex() << '\t' << (entry.isRedirect() ? 'R' : 'A');

  if (entry.isRedirect()) {
    std::cout << '\t' << entry.getRedirectEntry().getIndex();
  } else {
    auto item = entry.getItem();
    std::cout << '\t' << item.getMimetype() << '\t' << item.getSize();
  }
  std::cout << std::endl;
}

int ZimDumper::listEntriesByNamespace(const std::string ns, bool details)
{
  int ret = 0;
  for (auto& entry : m_archive.findByPath(ns)) {
    if (details) {
      ret = listEntry(entry);
    } else {
      std::cout << entry.getPath() << '\n';
    }
  }
  return ret;
}

void write_to_error_directory(std::string_view base,
                              std::string_view relpath,
                              std::string_view content)
{
  namespace fs = std::filesystem;

  createdir(ERRORSDIR, base);
  std::string url{relpath};

  std::string::size_type p;
  while ((p = url.find('/')) != std::string::npos) {
    url.replace(p, 1, "%2f");
  }

  fs::path fsBase{base};
  fs::path fsRelpath{relpath};
  fs::path fsPath = fsBase / ERRORSDIR / fsRelpath;

#ifdef _WIN32
  std::wstring wpath = fsPath.wstring();
  auto fd = _wopen(wpath.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC, S_IWRITE);

  if (fd == -1) {
    std::cerr << "Error opening file " + fullpath
                     + " cause: " + ::strerror(errno)
              << std::endl;
    return;
  }
  if ((size_t)write(fd, content, content.size()) != content.size()) {
    close(fd);
    std::cerr << "Failed writing: " << fullpath << " - " << ::strerror(errno)
              << std::endl;
  }
#else
  std::ofstream stream(fsPath);

  stream.write(content.data(), content.size());

  if (stream.fail() || stream.bad()) {
    std::cerr << "Error writing file to errors dir. " << fsPath << std::endl;
    throw std::runtime_error(std::string("Error writing file to errors dir. ")
                                 .append(fsPath.native()));
  } else {
    std::cerr << "Wrote " << (fsBase / fsRelpath) << " to " << fsPath
              << std::endl;
  }
#endif
}

inline void write_to_file(std::string_view base,
                          std::string_view path,
                          std::string_view data)
{
  namespace fs = std::filesystem;

  fs::path fsBase{base};
  fs::path fsPath{path};
  fs::path fullpath = fsBase / fsPath;

#ifdef _WIN32
  std::wstring wpath = fullpath.wstring();
  auto fd = _wopen(wpath.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC, S_IWRITE);
#else
  auto fd = ::open(fullpath.c_str(),
                   O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
  if (fd == -1) {
    write_to_error_directory(base, path, data);
    return;
  }
  if ((size_t)write(fd, data.data(), data.size()) != data.size()) {
    write_to_error_directory(base, path, data);
  }
  close(fd);
}

void ZimDumper::writeHttpRedirect(
    std::string_view directory,
    std::string_view outputPath,
    std::string_view /*currentEntryPath*/,  // unused
    std::string_view redirectPath)
{
  const auto content = httpRedirectHtml(redirectPath);
  write_to_file(directory,  // + SEPARATOR, // since we use std::filesystem see
                            // if it plays nice without
                outputPath,
                content);
}

void ZimDumper::dumpFiles(std::string_view directory,
                          bool symlinkdump,
                          std::function<bool(const char c)> /*nsfilter*/)
{
  namespace fs = std::filesystem;
  fs::path fsDirectory{directory};

  unsigned int truncatedFiles = 0;
  fs::create_directories(fsDirectory);

  // #if defined(_WIN32)
  //     std::wstring wdir = utf8ToUtf16(directory);
  //     CreateDirectoryW(wdir.c_str(), NULL);
  // #else
  //   ::mkdir(directory.c_str(), 0777);
  // #endif

  std::unordered_set<fs::path> pathCache;

  for (const zim::Entry& entry : m_archive.iterEfficient()) {
    fs::path entryPath = entry.getPath();
    fs::path entryDir = entryPath.parent_path();
    fs::path entryFilename = entryPath.filename();  // can check .empty

    if (not entryDir.empty()) {
      auto [itr, inserted] = pathCache.emplace(entryDir);
      if (inserted) {
        createdir(entryDir.c_str(), directory);
      }
    }

    if (entryFilename.native().size() > 255) {
      std::ostringstream sspostfix, sst;
      sspostfix << (++truncatedFiles);
      sst << entryFilename.native().substr(0, 254 - sspostfix.tellp()) << "~"
          << sspostfix.str();
      entryFilename = fs::path{sst.str()};
    }

    fs::path relativePath = entryDir / entryFilename;
    fs::path fullPath = fs::path{directory} / relativePath;

    if (entry.isRedirect()) {
      zim::Item redirectItem = entry.getItem(/*follow=*/true);
      fs::path redirectPath{redirectItem.getPath()};

      redirectPath
          = computeRelativePath(entryFilename.native(), redirectPath.native());

      constexpr static std::string_view htmlMimeType{"text/html"};

      if (!symlinkdump && redirectItem.getMimetype() == htmlMimeType) {
        writeHttpRedirect(fsDirectory.native(),
                          relativePath.native(),
                          entryPath.native(),
                          redirectPath.native());
      } else {
#ifdef _WIN32
        zim::Blob blob = redirectItem.getData();
        write_to_file(fsDirectory.native(),
                      relativePath.native(),
                      std::string_view{blob.data(), blob.size()});
#else
        if (::symlink(redirectPath.c_str(), fullPath.c_str()) != 0) {
          throw std::runtime_error(std::string("Error creating symlink from ")
                                   + fullPath.native() + " to "
                                   + redirectPath.native());
        }
#endif
      }
    } else {
        zim::Blob blob = entry.getItem().getData();
        write_to_file(fsDirectory.native(),
                      relativePath.native(),
                      std::string_view{blob.data(), blob.size()});
    }
  }
}

const static std::string USAGE =
    R"(
zimdump tool is used to inspect a zim file and also to dump its contents into the filesystem.

Usage:
  zimdump list [--details] [--idx=INDEX|([--url=URL] [--ns=N])] [--] <file>
  zimdump dump --dir=DIR [--ns=N] [--redirect] [--] <file>
  zimdump show (--idx=INDEX|(--url=URL [--ns=N])) [--] <file>
  zimdump info [--ns=N] [--] <file>
  zimdump -h | --help
  zimdump --version

Selectors:
  --idx INDEX  The index of the article to list/dump.
  --url URL    The url of the article to list/dump
  --ns N       The namespace of the article(s) to list/dump.
               When used with `--url`, default to `A`.
               If no `--url` is provided (for  `zimdump dump`) default to no filter.

Options:
  --details    Show details about the articles. Else, list only the url of the article(s).
  --dir=DIR    Directory where to dump the article(s) content.
  --redirect   Use symlink to dump redirect articles. Else create html redirect file
  -h, --help   Show this help
  --version    Show zimdump version.

Return value:
  0 : If no error
  1 : If no (or more than 1) articles correspond to the selectors.
  2 : If an error/warning has been issue during the dump.
      See DIR/dump_errors.log for the listing of the errors.
)";

// Older version of docopt doesn't define Options
using Options = std::map<std::string, docopt::value>;

int subcmdInfo(ZimDumper& app, Options& /*args*/)
{
  app.printInfo();
  return 0;
}

int subcmdDumpAll(ZimDumper& app,
                  const std::string& outdir,
                  bool redirect,
                  std::function<bool(const char c)> nsfilter)
{
#ifdef _WIN32
  app.dumpFiles(outdir, false, nsfilter);
#else
  app.dumpFiles(outdir, redirect, nsfilter);
#endif
  return 0;
}

int subcmdDump(ZimDumper& app, Options& args)
{
  bool redirect = args["--redirect"].asBool();

  std::function<bool(const char c)> filter
      = [](const char /*c*/) { return true; };
  if (args["--ns"]) {
    std::string nspace = args["--ns"].asString();
    filter = [nspace](const char c) { return nspace.at(0) == c; };
  }

  std::string directory = args["--dir"].asString();

  if (directory.empty()) {
    throw std::runtime_error("Directory cannot be empty.");
  }

  if (directory.back() == '/') {
    directory.pop_back();
  }

  return subcmdDumpAll(app, directory, redirect, filter);
}

zim::Entry getEntry(ZimDumper& app, Options& args)
{
  if (args["--idx"]) {
    return app.getEntry(args["--idx"].asLong());
  }

  const std::string entryPath = args["--url"].asString();
  const auto ns = args["--ns"];
  if (!ns) {
    return app.getEntryByPath(entryPath);
  }

  return app.getEntryByNsAndPath(ns.asString()[0], entryPath);
}

int subcmdShow(ZimDumper& app, Options& args)
{
  // docopt guaranty us that we have `--idx` or `--url`.
  try {
    return app.dumpEntry(getEntry(app, args));
  } catch (...) {
    std::cerr << "Entry not found" << std::endl;
    return -1;
  }
}

int subcmdList(ZimDumper& app, Options& args)
{
  bool idx(args["--idx"]);
  bool url(args["--url"]);
  bool details = args["--details"].asBool();
  bool ns(args["--ns"]);

  if (idx || url) {
    try {
      // docopt guaranty us that we have `--idx` or `--url` (or nothing, but not
      // both)
      if (idx) {
        return app.listEntry(app.getEntry(args["--idx"].asLong()));
      } else {
        return app.listEntry(app.getEntryByPath(args["--url"].asString()));
      }
    } catch (...) {
      std::cerr << "Entry not found" << std::endl;
      return -1;
    }
  } else if (ns) {
    return app.listEntriesByNamespace(args["--ns"].asString(), details);
  } else {
    return app.listEntries(details);
  }
}

int main(int argc, char* argv[])
{
  int ret = 0;
  std::ostringstream versions;
  printVersions(versions);
  Options args
      = docopt::docopt(
          USAGE,
          {argv + 1, argv + argc},
          true,
          versions.str());

  try {
    ZimDumper app(args["<file>"].asString());

    std::unordered_map<std::string,
                       std::function<int(ZimDumper&, decltype(args)&)>>
        dispatchtable = {{"info", subcmdInfo},
                         {"dump", subcmdDump},
                         {"list", subcmdList},
                         {"show", subcmdShow}};

    // call the appropriate subcommand handler
    for (const auto& it : dispatchtable) {
      if (args[it.first.c_str()].asBool()) {
        ret = (it.second)(app, args);
        break;
      }
    }
  } catch (std::exception& e) {
    std::cout << "Exception: " << e.what() << '\n';
    return -1;
  }
  return ret;
}

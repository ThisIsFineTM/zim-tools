/*
 * Copyright (C) 2009 Tommi Maekitalo
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

#include <zim-tools/version.h>  // for printVersions

#include <zim/archive.h>        // for Archive
#include <zim/blob.h>           // for Blob
#include <zim/entry.h>          // for Entry
#include <zim/item.h>           // for Item

#include <getopt.h>             // for getopt_long, option
#include <unistd.h>             // for optarg, optind

#include <chrono>     // for duration, operator-, steady_clock
#include <cstdlib>    // for atoi, rand, srand
#include <ctime>      // for time
#include <exception>  // for exception
#include <iostream>   // for basic_ostream, char_traits, operator<<
#include <set>        // for _Rb_tree_const_iterator, operator==, set
#include <string>     // for basic_string, operator<<, string, ope...
#include <vector>     // for vector

std::string randomUrl()
{
  std::string url;
  for (unsigned n = 0; n < 10; ++n)
    url += static_cast<char>('A' + rand() % 26);
  return url;
}

void displayHelp()
{
  std::cerr
      << "\nzimbench benchmarks a ZIM file reading speed.\n\n"
         "usage: zimbench [options] zimfile\n"
         "\t-n number\tnumber of linear accessed articles (default 1000)\n"
         "\t-r number\tnumber of random accessed articles (default: same as "
         "-n)\n"
         "\t-d number\tnumber of distinct articles used for random access "
         "(default: same as -r)\n\n"
         "\t-v to print the software version\n"
      << std::flush;
}

int main(int argc, char* argv[])
{
  unsigned int count = 1000;
  bool randomCountSet = false;
  unsigned int randomCount = 1000;
  bool distinctCountSet = false;
  unsigned int distinctCount = 1000;
  std::string filename;

  static struct option long_options[] = {{0, 0, 0, 0}};

  try {
    while (true) {
      int option_index = 0;
      int c = getopt_long(argc, argv, "vn:r:d:", long_options, &option_index);

      if (c != -1) {
        switch (c) {
          case 'n':
            count = atoi(optarg);
            if (!randomCountSet) {
              randomCount = count;
              if (!distinctCountSet) {
                distinctCount = count;
              }
            }
            break;
          case 'r':
            randomCountSet = true;
            randomCount = atoi(optarg);
            if (!distinctCountSet) {
              distinctCount = count;
            }
            break;
          case 'd':
            distinctCountSet = true;
            distinctCount = atoi(optarg);
            break;
          case 'v':
            printVersions();
            return 0;
          case '?':
            std::cerr << "Unknown option `" << argv[optind - 1] << "'\n";
            displayHelp();
            return 1;
        };
      } else {
        if (optind < argc) {
          filename = argv[optind++];
        }
        break;
      }
    }

    if (filename.empty()) {
      displayHelp();
      return 1;
    }

    srand(time(0));

    std::cout << "open file " << filename << std::endl;
    zim::Archive archive(filename);

    // collect urls
    typedef std::set<std::string> UrlsType;
    typedef std::vector<std::string> RandomUrlsType;
    UrlsType urls;
    RandomUrlsType randomUrls;

    std::cout << "collect linear urls" << std::endl;
    for (auto& entry : archive.iterByPath()) {
      if (urls.size() >= count) {
        break;
      }
      std::cout << "check url " << entry.getPath() << '\t' << urls.size()
                << " found" << std::endl;
      if (!entry.isRedirect())
        urls.insert(entry.getPath());
    }

    std::cout << urls.size() << " urls collected" << std::endl;

    std::cout << "collect random urls" << std::endl;
    while (randomUrls.size() < distinctCount) {
      auto entry = archive.getEntryByPath(randomUrl());
      if (!entry.isRedirect())
        randomUrls.push_back(entry.getPath());
    }

    std::cout << randomUrls.size() << " random urls collected" << std::endl;

    // reopen file
    archive = zim::Archive(filename);

    // linear read
    std::cout << "linear:" << std::flush;
    auto start = std::chrono::steady_clock::now();

    unsigned size = 0;
    for (UrlsType::const_iterator it = urls.begin(); it != urls.end(); ++it) {
      try {
        auto entry = archive.getEntryByPath(*it);
        size += entry.getItem(true).getData().size();
      } catch (...) {
        std::cerr << "Impossible to get article '" << *it << "'" << std::endl;
      }
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "\tsize=" << size << "\tt=" << diff.count() << "s\t"
              << (static_cast<double>(urls.size()) / diff.count())
              << " articles/s" << std::endl;

    // reopen file
    archive = zim::Archive(filename);

    // random access
    std::cout << "random:" << std::flush;

    start = std::chrono::steady_clock::now();

    size = 0;
    for (unsigned r = 0; r < randomCount; ++r) {
      try {
        auto entry
            = archive.getEntryByPath(randomUrls[rand() % randomUrls.size()]);
        size += entry.getItem(true).getData().size();
      } catch (...) {
      }
    }
    // for (UrlsType::const_iterator it = randomUrls.begin(); it !=
    // randomUrls.end(); ++it) size += file.getArticle(ns,
    // *it).getData().size();

    end = std::chrono::steady_clock::now();
    diff = end - start;
    std::cout << "\tsize=" << size << "\tt=" << diff.count() << "s\t"
              << (static_cast<double>(randomCount) / diff.count())
              << " articles/s" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}


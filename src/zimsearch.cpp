/*
 * Copyright (C) 2007 Tommi Maekitalo
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

#include <zim-tools/version.h>    // for printVersions
                                  //
#include <zim/archive.h>          // for Archive
#include <zim/search.h>           // for Query, Search, SearchResultSet, Sea...
#include <zim/search_iterator.h>  // for SearchIterator
                                  //
#include <exception>              // for exception
#include <iostream>               // for basic_ostream, operator<<, endl
#include <string>                 // for char_traits, basic_string, operator==

void printSearchResults(zim::Search& search)
{
  auto results = search.getResults(0, search.getEstimatedMatches());
  for (auto it = results.begin(); it != results.end(); ++it) {
    std::cout << "article " << it->getIndex() << "\nscore " << it.getScore()
              << "\t:\t" << it.getTitle() << std::endl;
  }
}

int main(int argc, char* argv[])
{
  try {
    // version number
    if (argc > 1
        && (std::string(argv[1]) == "-v"
            || std::string(argv[1]) == "--version")) {
      printVersions();
      return 0;
    }

    if (argc <= 2) {
      std::cerr << "\nzimsearch allows to search content in a ZIM file.\n\n"
                   "usage: "
                << argv[0]
                << " [-x indexfile] zimfile searchstring\n"
                   "\n"
                   "options\n"
                   "  -x indexfile   specify indexfile\n"
                   "  -v, --version  print software version\n"
                << std::endl;
      return 1;
    }

    std::string s = argv[2];
    for (int a = 3; a < argc; ++a) {
      s += ' ';
      s += argv[a];
    }

    zim::Archive zimarchive(argv[1]);
    zim::Searcher searcher(zimarchive);
    zim::Query query;
    query.setQuery(s);
    auto search = searcher.search(query);
    printSearchResults(search);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}


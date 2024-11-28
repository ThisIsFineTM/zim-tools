/*
 * Copyright (C) Emmanuel Engelhart <kellson@kiwix.org>
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

#ifndef ZIM_TOOL_VERSION_H_
#define ZIM_TOOL_VERSION_H_

#include <iostream> // requires transitive include. Need to fix in libzim.

#include <zim/version.h>

#ifndef VERSION
#define VERSION "undefined"
#endif

inline void printVersions(std::ostream& out = std::cout)
{
  out << "zim-tools " << VERSION << '\n';
  out << '\n';
  zim::printVersions(out);
}

#endif  // ZIM_TOOL_VERSION_H_

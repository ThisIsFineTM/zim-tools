/*
 * Copyright 2023 Veloman Yunkan <veloman.yunkan@gmail.com>
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

#ifndef OPENZIM_METADATA_H
#define OPENZIM_METADATA_H

#include <cstddef>  // for size_t
#include <map>      // for map
#include <string>   // for basic_string, string, operator<=>
#include <vector>   // for vector

namespace zim
{

class Metadata
{
  using KeyValueMap = std::map<std::string, std::string>;

 public:  // types
  struct ReservedMetadataRecord {
    const std::string name;
    const bool isMandatory;
    const size_t minLength;
    const size_t maxLength;
    const std::string regex;
  };

  using ReservedMetadataTable = std::vector<ReservedMetadataRecord>;

  using Errors = std::vector<std::string>;

  using Iterator = KeyValueMap::const_iterator;

  // data
  static const ReservedMetadataTable& reservedMetadataInfo;

  // functions
  void set(const std::string& name, const std::string& value);
  [[nodiscard]] bool has(const std::string& name) const;
  const std::string& operator[](const std::string& name) const;

  [[nodiscard]] bool valid() const;
  [[nodiscard]] Errors check() const;

  static const ReservedMetadataRecord& getReservedMetadataRecord(
      const std::string& name);

  [[nodiscard]] Iterator begin() const { return data.begin(); }
  [[nodiscard]] Iterator end() const { return data.end(); }

 private:  // functions
  [[nodiscard]] Errors checkMandatoryMetadata() const;
  [[nodiscard]] Errors checkSimpleConstraints() const;
  [[nodiscard]] Errors checkComplexConstraints() const;

  // data
  KeyValueMap data;
};

}  // namespace zim

#endif  //  OPENZIM_METADATA_H

/*
 * This file is part of wysiwidi
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#ifndef PROPERTY_H_
#define PROPERTY_H_

#include <string>
#include <map>

#include "constants.h"

namespace WFD {

class Property {
 public:
  explicit Property(PropertyType type);
  virtual ~Property();
  virtual std::string to_string() const;

  PropertyType type() { return type_; }
  bool is_none() const { return is_none_; }

 protected:
  Property(PropertyType type, bool is_none_);

 private:
  PropertyType type_;
  bool is_none_;
};

}  // namespace WFD

#endif  // PROPERTY_H_

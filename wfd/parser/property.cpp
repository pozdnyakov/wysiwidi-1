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


#include "property.h"

namespace WFD {

Property::Property(PropertyType type)
    : type_(type),
      is_none_(false) {
}

Property::Property(PropertyType type, bool is_none)
    : type_(type),
      is_none_(is_none) {
}

Property::~Property() {
	// TODO Auto-generated destructor stub
}

std::string Property::to_string() const {
  return std::string();
}

}  // namespace WFD

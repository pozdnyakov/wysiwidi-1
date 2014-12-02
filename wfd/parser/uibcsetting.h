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


#ifndef UIBCSETTING_H_
#define UIBCSETTING_H_

#include "property.h"

namespace WFD {

class UIBCSetting : public Property {
 public:
  explicit UIBCSetting(bool is_enabled);
  virtual ~UIBCSetting();

  bool is_enabled() const { return is_enabled_; }
  virtual std::string to_string() const override;

 private:
  bool is_enabled_;
};

}  // namespace WFD

#endif  // UIBCSETTING_H_

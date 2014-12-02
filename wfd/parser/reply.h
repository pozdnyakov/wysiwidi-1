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


#ifndef REPLY_H_
#define REPLY_H_

#include "message.h"

namespace WFD {

class Reply: public Message {
 public:
  explicit Reply(int response_code);
  virtual ~Reply();

  int response_code() const { return response_code_; }
  void set_response_code(int response_code) { response_code_ = response_code; }

  virtual std::string to_string() const override;

 private:
  int response_code_;
};

}  // namespace WFD

#endif  // REPLY_H_

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


#ifndef CLIENTRTPPORTS_H_
#define CLIENTRTPPORTS_H_

#include "property.h"

namespace WFD {

class ClientRtpPorts: public Property {
 public:
  ClientRtpPorts(unsigned short rtp_port_0, unsigned short rtp_port_1);
  virtual ~ClientRtpPorts();

  unsigned short rtp_port_0() const { return rtp_port_0_; }
  unsigned short rtp_port_1() const { return rtp_port_1_; }
  virtual std::string to_string() const override;

 private:
  unsigned short rtp_port_0_;
  unsigned short rtp_port_1_;
};

}  // namespace WFD

#endif  // CLIENTRTPPORTS_H_

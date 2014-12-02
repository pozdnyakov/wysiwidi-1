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


#include "play.h"

namespace WFD {

Play::Play(const std::string& request_uri)
 : Message(MessageTypePlay, request_uri) {
}

Play::~Play() {
}

std::string Play::to_string() const {
  std::string ret = MethodName::PLAY
      + std::string(SPACE) + request_uri()
      + std::string(SPACE) + std::string(RTSP_END) + std::string(CRLF);
  return ret + Message::to_string();
}

} /* namespace WFD */

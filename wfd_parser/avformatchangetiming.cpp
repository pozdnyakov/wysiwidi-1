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


#include "avformatchangetiming.h"

#include "macros.h"

namespace WFD {

AVFormatChangeTiming::AVFormatChangeTiming(unsigned long long int pts,
    unsigned long long int dts)
  : Property(WFD_AV_FORMAT_CHANGE_TIMING),
    pts_(pts),
    dts_(dts) {

}

AVFormatChangeTiming::~AVFormatChangeTiming() {
}

std::string AVFormatChangeTiming::to_string() const {
  MAKE_HEX_STRING_10(pts, pts_);
  MAKE_HEX_STRING_10(dts, dts_);
  std::string ret =
      PropertyName::wfd_av_format_change_timing + std::string(SEMICOLON)
     + std::string(SPACE) + pts
     + std::string(SPACE) + dts;
  return ret;
}

}  // namespace WFD

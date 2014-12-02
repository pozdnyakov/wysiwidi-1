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


#include "formats3d.h"

#include "macros.h"

namespace WFD {

Formats3d::Formats3d() : Property(WFD_3D_FORMATS, true) {
}

Formats3d::Formats3d(unsigned char native,
    unsigned char preferred_display_mode,
    const H264Codecs3d& h264_codecs_3d)
  : Property(WFD_3D_FORMATS),
    native_(native),
    preferred_display_mode_(preferred_display_mode),
    h264_codecs_3d_(h264_codecs_3d) {
}

Formats3d::~Formats3d() {
  // TODO Auto-generated destructor stub
}

std::string H264Codec3d::to_string() const {
  std::string ret;
  MAKE_HEX_STRING_2(profile, profile_);
  MAKE_HEX_STRING_2(level, level_);
  MAKE_HEX_STRING_16(video_capability_3d, video_capability_3d_);
  MAKE_HEX_STRING_2(latency,latency_);
  MAKE_HEX_STRING_4(min_slice_size, min_slice_size_);
  MAKE_HEX_STRING_4(slice_enc_params, slice_enc_params_);
  MAKE_HEX_STRING_2(frame_rate_control_support, frame_rate_control_support_);

  ret = profile + std::string(SPACE)
      + level + std::string(SPACE)
      + video_capability_3d + std::string(SPACE)
      + latency + std::string(SPACE)
      + min_slice_size + std::string(SPACE)
      + slice_enc_params + std::string(SPACE)
      + frame_rate_control_support + std::string(SPACE);

  if (max_hres_ >= 0) {
    MAKE_HEX_STRING_4(max_hres, max_hres_);
    ret += max_hres;
  } else {
    ret += NONE;
  }
  ret += std::string(SPACE);

  if (max_vres_ >= 0) {
    MAKE_HEX_STRING_4(max_vres, max_vres_);
    ret += max_vres;
  } else {
    ret += NONE;
  }

  return ret;
}

std::string Formats3d::to_string() const {
  std::string ret;

  ret = PropertyName::wfd_3d_formats
      + std::string(SEMICOLON)+ std::string(SPACE);

  if (is_none())
    return ret + WFD::NONE;

  MAKE_HEX_STRING_2(native, native_);
  MAKE_HEX_STRING_2(preferred_display_mode, preferred_display_mode_);

  ret += native + std::string(SPACE)
      + preferred_display_mode + std::string(SPACE);

  auto it = h264_codecs_3d_.begin();
  auto end = h264_codecs_3d_.end();
  while(it != end) {
    ret += (*it).to_string();
    ++it;
    if (it != end)
      ret += ", ";
  }

  return ret;
}

}  // namespace WFD

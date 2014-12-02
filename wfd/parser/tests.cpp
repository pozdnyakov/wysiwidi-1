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


#include <list>
#include <algorithm>
#include "constants.h"
#include "driver.h"
#include "reply.h"
#include "audiocodecs.h"
#include "avformatchangetiming.h"
#include "clientrtpports.h"
#include "connectortype.h"
#include "contentprotection.h"
#include "displayedid.h"
#include "formats3d.h"
#include "i2c.h"
#include "presentationurl.h"
#include "triggermethod.h"
#include "uibcsetting.h"
#include "videoformats.h"
#include "propertyerrors.h"

typedef bool (*TestFunc)(void);

#define ASSERT_EQUAL(value, expected) \
  if ((value) != (expected)) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << #value << ": " \
              << "expected '" << (expected) \
              << "', got '" << (value) << "'" \
              << std::endl; \
    return 0; \
  }

#define ASSERT(assertion) \
  if (!(assertion)) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "assertion failed: " << #assertion \
              << std::endl; \
    return 0; \
  }

#define ASSERT_NO_EXCEPTION(method_call) \
  try { \
    method_call; \
  } catch (std::exception &x) { \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "unexpected exception: " << #method_call << ": " \
              << x.what() << std::endl; \
    return false; \
  }

#define ASSERT_EXCEPTION(method_call) \
  try { \
    method_call; \
    std::cout << __func__ << " (" << __FILE__ << ":" << __LINE__ << "): " \
              << "expected exception: " << #method_call << std::endl; \
    return false; \
  } catch (std::exception &x) { \
    ; \
  }

static bool property_type_exists (std::vector<std::string> properties,
                                  WFD::PropertyType type)
{
  return std::find (properties.begin(),
                    properties.end(),
                    WFD::PropertyName::name[type]) != properties.end();
}

static bool test_audio_codec (WFD::AudioCodec codec, WFD::AudioFormat::Type format,
                              unsigned long int modes, unsigned char latency)
{
  ASSERT_EQUAL(codec.audio_format(), format);
  ASSERT_EQUAL(codec.audio_modes().to_ulong(), modes);
  ASSERT_EQUAL(codec.latency(), latency);

  return true;
}

static bool test_h264_codec (WFD::H264Codec codec,
                             unsigned char profile, unsigned char level,
                             unsigned int cea_support, unsigned int vesa_support,
                             unsigned int hh_support, unsigned char latency,
                             unsigned short min_slice_size, unsigned short slice_enc_params,
                             unsigned char frame_rate_control_support, int max_hres,
                             int max_vres)
{
  ASSERT_EQUAL(codec.profile_, profile);
  ASSERT_EQUAL(codec.level_, level);
  ASSERT_EQUAL(codec.cea_support_, cea_support);
  ASSERT_EQUAL(codec.vesa_support_, vesa_support);
  ASSERT_EQUAL(codec.hh_support_, hh_support);
  ASSERT_EQUAL(codec.latency_, latency);
  ASSERT_EQUAL(codec.min_slice_size_, min_slice_size);
  ASSERT_EQUAL(codec.slice_enc_params_, slice_enc_params);
  ASSERT_EQUAL(codec.frame_rate_control_support_, frame_rate_control_support);

  // TODO test max-hres and max-vres

  return true;
}

static bool test_h264_codec_3d (WFD::H264Codec3d codec,
                                unsigned char profile, unsigned char level,
                                unsigned long long int video_capability_3d, unsigned char latency,
                                unsigned short min_slice_size, unsigned short slice_enc_params,
                                unsigned char frame_rate_control_support, int max_hres,
                                int max_vres)
{
  ASSERT_EQUAL(codec.profile_, profile);
  ASSERT_EQUAL(codec.level_, level);
  ASSERT_EQUAL(codec.video_capability_3d_, video_capability_3d);
  ASSERT_EQUAL(codec.latency_, latency);
  ASSERT_EQUAL(codec.min_slice_size_, min_slice_size);
  ASSERT_EQUAL(codec.slice_enc_params_, slice_enc_params);
  ASSERT_EQUAL(codec.frame_rate_control_support_, frame_rate_control_support);

  // TODO test max-hres and max-vres

  return true;
}

static bool test_valid_options ()
{
  WFD::Driver driver;

  std::string header("OPTIONS * RTSP/1.0\r\n"
                     "CSeq: 0\r\n"
                     "Require: org.wfa.wfd1.0\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeOptions);
  ASSERT_EQUAL(wfd_message->header().cseq(), 0);
  ASSERT_EQUAL(wfd_message->header().content_length(), 0);
  ASSERT_EQUAL(wfd_message->header().require_wfd_support(), true);

  ASSERT_EQUAL (wfd_message->to_string(), header);

  return true;
}

static bool test_valid_options_reply ()
{
  WFD::Driver driver;

  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 1\r\n"
                     "Public: org.wfa.wfd1.0, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);

  std::shared_ptr<WFD::Reply> reply = std::dynamic_pointer_cast<WFD::Reply>(wfd_message);
  ASSERT(reply != NULL);

  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->type(), WFD::Message::MessageTypeReply);
  ASSERT_EQUAL(reply->header().cseq(), 1);
  ASSERT_EQUAL(reply->header().content_length(), 0);

  std::vector<WFD::Method> methods = wfd_message->header().supported_methods();
  static const WFD::Method expected[] = { WFD::Method::ORG_WFA_WFD_1_0,
                                          WFD::Method::SETUP,
                                          WFD::Method::TEARDOWN,
                                          WFD::Method::PLAY,
                                          WFD::Method::PAUSE,
                                          WFD::Method::GET_PARAMETER,
                                          WFD::Method::SET_PARAMETER };

  ASSERT_EQUAL(methods.size(), 7);
  for (int i = 0; i < 7; i++) {
    std::vector<WFD::Method>::iterator method = std::find(methods.begin(), methods.end(), expected[i]);
    ASSERT(method != methods.end());
  }

  ASSERT_EQUAL (reply->to_string(), header);

  return true;
}

static bool test_valid_extra_properties ()
{
  WFD::Driver driver;

  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Length: 72\r\n"
                     "My-Header: 123 testing testing\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  GenericHeaderMap gen_headers = driver.parsed_message()->header().generic_headers();
  auto extra_header_it = gen_headers.find("My-Header");
  ASSERT(extra_header_it != gen_headers.end())
  ASSERT_EQUAL(extra_header_it->second, "123 testing testing");
  ASSERT_EQUAL(driver.parsed_message()->header().content_length(), 72);

  std::string message("nonstandard_property: 1!!1! non standard value\r\n"
                      "wfd_audio_codecs: none\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  auto payload = driver.parsed_message()->payload();
  std::shared_ptr<WFD::Property> property;

  ASSERT_NO_EXCEPTION (property =
      payload.get_property(WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT(property->is_none());

  ASSERT_NO_EXCEPTION (property =
      payload.get_property("nonstandard_property"));
  auto extra_property = std::static_pointer_cast<WFD::GenericProperty> (property);
  ASSERT_EQUAL(extra_property->value(), "1!!1! non standard value");

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_extra_errors ()
{
  WFD::Driver driver;

  std::string header("RTSP/1.0 303 OK\r\n"
                     "CSeq: 0\r\n"
                     "Content-Length: 55\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  std::string message("wfd_audio_codecs: 103\r\n"
                      "nonstandard_property: 101, 102\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  auto payload = driver.parsed_message()->payload();
  std::shared_ptr<WFD::PropertyErrors> error;

  ASSERT_EQUAL (payload.property_errors().size(), 2);

  ASSERT_NO_EXCEPTION(error =
      payload.get_property_error(WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT_EQUAL(error->error_codes().size(), 1);
  ASSERT_EQUAL(error->error_codes()[0], 103);

  ASSERT_NO_EXCEPTION(error =
      payload.get_property_error("nonstandard_property"));
  ASSERT_EQUAL(error->error_codes().size(), 2);
  ASSERT_EQUAL(error->error_codes()[0], 101);
  ASSERT_EQUAL(error->error_codes()[1], 102);

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_extra_properties_in_get ()
{
  WFD::Driver driver;

  std::string header("GET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 40\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  std::string message("nonstandard_property\r\n"
                      "wfd_audio_codecs\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  auto properties = driver.parsed_message()->payload().get_parameter_properties();
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT_EQUAL(properties.size(), 2);
  ASSERT_EQUAL(properties[0], "nonstandard_property");
  ASSERT_EQUAL(properties[1], "wfd_audio_codecs");

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_get_parameter ()
{
  WFD::Driver driver;

  std::string header("GET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 213\r\n\r\n");
  std::string message("wfd_client_rtp_ports\r\n"
                      "wfd_audio_codecs\r\n"
                      "wfd_video_formats\r\n"
                      "wfd_3d_video_formats\r\n"
                      "wfd_coupled_sink\r\n"
                      "wfd_display_edid\r\n"
                      "wfd_connector_type\r\n"
                      "wfd_uibc_capability\r\n"
                      "wfd_standby_resume_capability\r\n"
                      "wfd_content_protection\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeGetParameter);
  ASSERT_EQUAL(wfd_message->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(wfd_message->header().cseq(), 2);
  ASSERT_EQUAL(wfd_message->header().content_length(), 213);
  ASSERT_EQUAL(wfd_message->header().content_type(), "text/parameters");
  ASSERT_EQUAL(wfd_message->header().require_wfd_support(), false);

  std::vector<std::string> properties = wfd_message->payload().get_parameter_properties();
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_CLIENT_RTP_PORTS));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_CLIENT_RTP_PORTS));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_VIDEO_FORMATS));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_3D_FORMATS));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_COUPLED_SINK));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_DISPLAY_EDID));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_CONNECTOR_TYPE));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_UIBC_CAPABILITY));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_STANDBY_RESUME_CAPABILITY));
  ASSERT(property_type_exists (properties, WFD::PropertyType::WFD_CONTENT_PROTECTION));

  ASSERT_EQUAL (wfd_message->to_string(), header + message)

  return true;
}

static bool test_valid_get_parameter_reply_with_all_none ()
{
  WFD::Driver driver;
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 483\r\n\r\n");
  // not a real-world message, just a collection of all properties
  std::string message("wfd_3d_video_formats: none\r\n"
                      "wfd_I2C: none\r\n"
                      "wfd_audio_codecs: none\r\n"
                      "wfd_av_format_change_timing: 000000000F 00000000FF\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_connector_type: none\r\n"
                      "wfd_content_protection: none\r\n"
                      "wfd_coupled_sink: none\r\n"
                      "wfd_display_edid: none\r\n"
                      "wfd_presentation_url: none none\r\n"
                      "wfd_route: primary\r\n"
                      "wfd_standby_resume_capability: none\r\n"
                      "wfd_trigger_method: TEARDOWN\r\n"
                      "wfd_uibc_capability: none\r\n"
                      "wfd_uibc_setting: disable\r\n"
                      "wfd_video_formats: none\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeReply);

  std::shared_ptr<WFD::Reply> reply = std::dynamic_pointer_cast<WFD::Reply>(wfd_message);
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 2);
  ASSERT_EQUAL(reply->header().content_length(), 483);
  ASSERT_EQUAL(reply->header().content_type(), "text/parameters");
  ASSERT_EQUAL(reply->header().supported_methods().size(), 0);

  auto payload = wfd_message->payload();
  std::shared_ptr<WFD::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_VIDEO_FORMATS));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_3D_FORMATS));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CONTENT_PROTECTION));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_DISPLAY_EDID));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_COUPLED_SINK));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_UIBC_CAPABILITY));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CONNECTOR_TYPE));
  ASSERT(prop->is_none());
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_STANDBY_RESUME_CAPABILITY));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_AV_FORMAT_CHANGE_TIMING));
  auto av_format_change_timing = std::static_pointer_cast<WFD::AVFormatChangeTiming> (prop);
  ASSERT_EQUAL(av_format_change_timing->pts(), 0x000000000F);
  ASSERT_EQUAL(av_format_change_timing->dts(), 0x00000000FF);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CLIENT_RTP_PORTS));
  auto client_rtp_ports = std::static_pointer_cast<WFD::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_TRIGGER_METHOD));
  auto trigger_method = std::static_pointer_cast<WFD::TriggerMethod> (prop);
  ASSERT_EQUAL(trigger_method->method(), WFD::TriggerMethod::TEARDOWN);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_PRESENTATION_URL));
  auto presentation_url = std::static_pointer_cast<WFD::PresentationUrl> (prop);
  ASSERT_EQUAL(presentation_url->presentation_url_1(), "");
  ASSERT_EQUAL(presentation_url->presentation_url_2(), "");

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_ROUTE));
  auto route = std::static_pointer_cast<WFD::Route> (prop);
  ASSERT_EQUAL(route->destination(), WFD::Route::PRIMARY);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_I2C));
  auto i2c = std::static_pointer_cast<WFD::I2C> (prop);
  ASSERT_EQUAL(i2c->is_supported(), false);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_UIBC_SETTING));
  auto uibc_setting = std::static_pointer_cast<WFD::UIBCSetting> (prop);
  ASSERT_EQUAL(uibc_setting->is_enabled(), false);

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_get_parameter_reply ()
{
  WFD::Driver driver;
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 535\r\n\r\n");
  std::string message("wfd_3d_video_formats: 80 00 03 0F 0000000000000005 00 0001 1401 13 none none\r\n"
                      "wfd_I2C: 404\r\n"
                      "wfd_audio_codecs: LPCM 00000003 00, AAC 00000001 00\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_connector_type: 05\r\n"
                      "wfd_content_protection: HDCP2.1 port=1189\r\n"
                      "wfd_coupled_sink: none\r\n"
                      "wfd_display_edid: none\r\n"
                      "wfd_standby_resume_capability: supported\r\n"
                      "wfd_uibc_capability: none\r\n"
                      "wfd_video_formats: 40 00 02 04 0001DEFF 053C7FFF 00000FFF 00 0000 0000 11 none none, 01 04 0001DEFF 053C7FFF 00000FFF 00 0000 0000 11 none none\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeReply);

  std::shared_ptr<WFD::Reply> reply = std::dynamic_pointer_cast<WFD::Reply>(wfd_message);
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->header().cseq(), 2);
  ASSERT_EQUAL(reply->header().content_length(), 535);
  ASSERT_EQUAL(reply->header().content_type(), "text/parameters");
  ASSERT_EQUAL(reply->header().supported_methods().size(), 0);

  auto payload = wfd_message->payload();
  std::shared_ptr<WFD::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_AUDIO_CODECS));

  // Test that all properties exist
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_AUDIO_CODECS));
  std::shared_ptr<WFD::AudioCodecs> audio_codecs = std::static_pointer_cast<WFD::AudioCodecs> (prop);
  ASSERT_EQUAL(audio_codecs->audio_codecs().size(), 2);
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[0],
                           WFD::AudioFormat::LPCM, 3, 0));
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[1],
                           WFD::AudioFormat::AAC, 1, 0));
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_VIDEO_FORMATS));
  std::shared_ptr<WFD::VideoFormats> video_formats = std::static_pointer_cast<WFD::VideoFormats> (prop);
  ASSERT_EQUAL(video_formats->native_resolution(), 0x40);
  ASSERT_EQUAL(video_formats->preferred_display_mode(), 0);
  ASSERT_EQUAL(video_formats->h264_codecs().size(), 2);
  ASSERT(test_h264_codec (video_formats->h264_codecs()[0],
                          0x02, 0x04, 0x0001DEFF, 0x053C7FFF, 0x00000FFF, 0, 0, 0, 0x11, 0, 0));
  ASSERT(test_h264_codec (video_formats->h264_codecs()[1],
                          0x01, 0x04, 0x0001DEFF, 0x053C7FFF, 0x00000FFF, 0, 0, 0, 0x11, 0, 0));
  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_3D_FORMATS));
  std::shared_ptr<WFD::Formats3d> formats_3d = std::static_pointer_cast<WFD::Formats3d> (prop);
  ASSERT_EQUAL(formats_3d->native_resolution(), 0x80);
  ASSERT_EQUAL(formats_3d->preferred_display_mode(), 0);
  ASSERT_EQUAL(formats_3d->codecs().size(), 1);
  ASSERT(test_h264_codec_3d (formats_3d->codecs()[0],
                             0x03, 0x0F, 0x0000000000000005, 0, 0x0001, 0x1401, 0x13, 0, 0));

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CONTENT_PROTECTION));
  std::shared_ptr<WFD::ContentProtection> content_protection = std::static_pointer_cast<WFD::ContentProtection> (prop);
  ASSERT_EQUAL(content_protection->hdcp_spec(), WFD::ContentProtection::HDCP_SPEC_2_1);
  ASSERT_EQUAL(content_protection->port(), 1189);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_DISPLAY_EDID));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_COUPLED_SINK));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CLIENT_RTP_PORTS));
  std::shared_ptr<WFD::ClientRtpPorts> client_rtp_ports = std::static_pointer_cast<WFD::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_UIBC_CAPABILITY));
  ASSERT(prop->is_none());

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CONNECTOR_TYPE));
  std::shared_ptr<WFD::ConnectorType> connector_type = std::static_pointer_cast<WFD::ConnectorType> (prop);
  ASSERT_EQUAL(connector_type->connector_type(), 5);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_I2C));
  auto i2c = std::static_pointer_cast<WFD::I2C> (prop);
  ASSERT_EQUAL(i2c->is_supported(), true);
  ASSERT_EQUAL(i2c->port(), 404);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_STANDBY_RESUME_CAPABILITY));
  ASSERT(!prop->is_none());

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_invalid_property_value ()
{
  WFD::Driver driver;
  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 1187\r\n");
  std::string message("wfd_uibc_capability: none and something completely different\r\n");

  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  ASSERT_EXCEPTION (driver.parse_payload(message));
 
  return true;
}

static bool test_case_insensitivity ()
{
  WFD::Driver driver;

  std::string invalid_header("OptionS * RTSP/1.0\r\n"
                             "CSeq: 0\r\n"
                             "Require: org.wfa.wfd1.0\r\n\r\n");
  ASSERT_EXCEPTION (driver.parse_header(invalid_header));

  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSEQ: 2\r\n"
                     "Content-Type: tEXT/parameters\r\n"
                     "Content-LENGTH: 1187\r\n");
  std::string message("wfd_uibc_capABILITY: noNE\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);

  ASSERT_EQUAL(wfd_message->header().cseq(), 2);
  ASSERT_EQUAL(wfd_message->header().content_type(), "tEXT/parameters");
  ASSERT_EQUAL(wfd_message->header().content_length(), 1187);

  auto payload = wfd_message->payload();
  std::shared_ptr<WFD::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_UIBC_CAPABILITY));
  ASSERT(prop->is_none());

  // TODO test insensitivity of triggers and method list

  return true;
}

static bool test_valid_get_parameter_reply_with_errors ()
{
  WFD::Driver driver;
  std::string header("RTSP/1.0 303 OK\r\n"
                     "CSeq: 2\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 42\r\n\r\n");
  std::string message("wfd_audio_codecs: 415, 457\r\n"
                      "wfd_I2C: 404\r\n");

  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  auto wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeReply);

  auto reply = std::static_pointer_cast<WFD::Reply>(wfd_message);
  ASSERT(reply != NULL);
  ASSERT_EQUAL(reply->response_code(), 303);

  auto payload = wfd_message->payload();
  std::shared_ptr<WFD::PropertyErrors> error;

  ASSERT_EQUAL (payload.property_errors().size(), 2);

  ASSERT_NO_EXCEPTION(error =
      payload.get_property_error(WFD::PropertyType::WFD_AUDIO_CODECS));
  ASSERT_EQUAL(error->error_codes().size(), 2);
  ASSERT_EQUAL(error->error_codes()[0], 415);
  ASSERT_EQUAL(error->error_codes()[1], 457);

  ASSERT_NO_EXCEPTION(error =
      payload.get_property_error(WFD::PropertyType::WFD_I2C));
  ASSERT_EQUAL(error->error_codes().size(), 1);
  ASSERT_EQUAL(error->error_codes()[0], 404);

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_set_parameter ()
{
  WFD::Driver driver;

  std::string header("SET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 3\r\n"
                     "Content-Type: text/parameters\r\n"
                     "Content-Length: 275\r\n\r\n");
  std::string message("wfd_audio_codecs: AAC 00000001 00\r\n"
                      "wfd_client_rtp_ports: RTP/AVP/UDP;unicast 19000 0 mode=play\r\n"
                      "wfd_presentation_url: rtsp://192.168.173.1/wfd1.0/streamid=0 none\r\n"
                      "wfd_trigger_method: SETUP\r\n"
                      "wfd_video_formats: 00 00 02 04 00000020 00000000 00000000 00 0000 0000 11 none none\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));
  ASSERT_NO_EXCEPTION (driver.parse_payload(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeSetParameter);
  ASSERT_EQUAL(wfd_message->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(wfd_message->header().cseq(), 3);
  ASSERT_EQUAL(wfd_message->header().content_length(), 275);
  ASSERT_EQUAL(wfd_message->header().require_wfd_support(), false);

  auto payload = wfd_message->payload();
  std::shared_ptr<WFD::Property> prop;

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_AUDIO_CODECS));
  std::shared_ptr<WFD::AudioCodecs> audio_codecs = std::static_pointer_cast<WFD::AudioCodecs> (prop);
  ASSERT_EQUAL(audio_codecs->audio_codecs().size(), 1);
  ASSERT(test_audio_codec (audio_codecs->audio_codecs()[0],
                           WFD::AudioFormat::AAC, 1, 0));

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_VIDEO_FORMATS));
  std::shared_ptr<WFD::VideoFormats> video_formats = std::static_pointer_cast<WFD::VideoFormats> (prop);
  ASSERT_EQUAL(video_formats->native_resolution(), 0);
  ASSERT_EQUAL(video_formats->preferred_display_mode(), 0);
  ASSERT_EQUAL(video_formats->h264_codecs().size(), 1);
  ASSERT(test_h264_codec (video_formats->h264_codecs()[0],
                          0x02, 0x04, 0x00000020, 0, 0, 0, 0, 0, 0x11, 0, 0));

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_CLIENT_RTP_PORTS));
  std::shared_ptr<WFD::ClientRtpPorts> client_rtp_ports = std::static_pointer_cast<WFD::ClientRtpPorts> (prop);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_0(), 19000);
  ASSERT_EQUAL(client_rtp_ports->rtp_port_1(), 0);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_TRIGGER_METHOD));
  std::shared_ptr<WFD::TriggerMethod> trigger_method = std::static_pointer_cast<WFD::TriggerMethod> (prop);
  ASSERT_EQUAL(trigger_method->method(), WFD::TriggerMethod::SETUP);

  ASSERT_NO_EXCEPTION (prop =
      payload.get_property(WFD::PropertyType::WFD_PRESENTATION_URL));

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header + message);

  return true;
}

static bool test_valid_setup ()
{
  WFD::Driver driver;

  std::string message("SETUP rtsp://10.82.24.140/wfd1.0/streamid=0 RTSP/1.0\r\n"
                      "CSeq: 4\r\n"
                      "Transport: RTP/AVP/UDP;unicast;client_port=19000\r\n"
                      "User-Agent: SEC-WDH/ME29\r\n"
                      "\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(message));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypeSetup);
  ASSERT_EQUAL(wfd_message->request_uri(), "rtsp://10.82.24.140/wfd1.0/streamid=0");
  ASSERT_EQUAL(wfd_message->header().cseq(), 4);
  ASSERT_EQUAL(wfd_message->header().content_length(), 0);
  ASSERT_EQUAL(wfd_message->header().require_wfd_support(), false);

  ASSERT_EQUAL(wfd_message->header().transport().client_port(), 19000);
  ASSERT_EQUAL(wfd_message->header().transport().client_supports_rtcp(), false);
  ASSERT_EQUAL(wfd_message->header().transport().server_port(), 0);
  ASSERT_EQUAL(wfd_message->header().transport().server_supports_rtcp(), false);

  ASSERT_EQUAL(driver.parsed_message()->to_string(), message);

  return true;
}

static bool test_valid_setup_reply ()
{
  WFD::Driver driver;

  std::string header("RTSP/1.0 200 OK\r\n"
                     "CSeq: 4\r\n"
                     "Session: 6B8B4567;timeout=30\r\n"
                     "Transport: RTP/AVP/UDP;unicast;client_port=19000;server_port=5000-5001\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);

  std::shared_ptr<WFD::Reply> reply = std::dynamic_pointer_cast<WFD::Reply>(wfd_message);
  ASSERT(reply != NULL);

  ASSERT_EQUAL(reply->response_code(), 200);
  ASSERT_EQUAL(reply->type(), WFD::Message::MessageTypeReply);
  ASSERT_EQUAL(wfd_message->header().cseq(), 4);
  ASSERT_EQUAL(wfd_message->header().content_length(), 0);
  ASSERT_EQUAL(wfd_message->header().session(), "6B8B4567");
  ASSERT_EQUAL(wfd_message->header().timeout(), 30);

  ASSERT_EQUAL(wfd_message->header().transport().client_port(), 19000);
  ASSERT_EQUAL(wfd_message->header().transport().client_supports_rtcp(), false);
  ASSERT_EQUAL(wfd_message->header().transport().server_port(), 5000);
  ASSERT_EQUAL(wfd_message->header().transport().server_supports_rtcp(), true);

  ASSERT_EQUAL(wfd_message->to_string(), header);

  return true;
}

static bool test_valid_play ()
{
  WFD::Driver driver;

  std::string header("PLAY rtsp://localhost/wfd1.0 RTSP/1.0\r\n"
                     "CSeq: 5\r\n"
                     "Session: 6B8B4567\r\n"
                     "User-Agent: SEC-WDH/ME29\r\n\r\n");
  ASSERT_NO_EXCEPTION (driver.parse_header(header));

  std::shared_ptr<WFD::Message> wfd_message(driver.parsed_message());
  ASSERT(wfd_message != NULL);
  ASSERT_EQUAL(wfd_message->type(), WFD::Message::MessageTypePlay);
  ASSERT_EQUAL(wfd_message->request_uri(), "rtsp://localhost/wfd1.0");
  ASSERT_EQUAL(wfd_message->header().cseq(), 5);
  ASSERT_EQUAL(wfd_message->header().content_length(), 0);
  ASSERT_EQUAL(wfd_message->header().require_wfd_support(), false);
  ASSERT_EQUAL(wfd_message->header().session(), "6B8B4567");

  ASSERT_EQUAL(driver.parsed_message()->to_string(), header);

  return true;
}

int main(const int argc, const char **argv)
{
  std::list<TestFunc> tests;
  int failures = 0;

  // Add tests
  tests.push_back(test_valid_options);
  tests.push_back(test_valid_options_reply);
  tests.push_back(test_valid_get_parameter);
  tests.push_back(test_valid_get_parameter_reply);
  tests.push_back(test_valid_get_parameter_reply_with_all_none);
  tests.push_back(test_valid_get_parameter_reply_with_errors);
  tests.push_back(test_valid_setup_reply);
  tests.push_back(test_valid_set_parameter);
  tests.push_back(test_valid_setup);
  tests.push_back(test_valid_play);
  tests.push_back(test_invalid_property_value);
  tests.push_back(test_case_insensitivity);
  tests.push_back(test_valid_extra_properties);
  tests.push_back(test_valid_extra_errors);
  tests.push_back(test_valid_extra_properties_in_get);

  // Run tests
  for (std::list<TestFunc>::iterator it=tests.begin(); it!=tests.end(); ++it) {
    TestFunc test = *it;
    if (!test())
      failures++;
  }

  if (failures > 0) {
    std::cout << std::endl << "Failed " << failures
              << " out of " << tests.size() << " tests" << std::endl;
    return 1;
  }

  std::cout << "Passed all " << tests.size() << " tests" << std::endl;
  return 0;
}

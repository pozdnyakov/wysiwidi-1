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


#include <memory>
#include <algorithm>
#include <assert.h>

#include "mirac-sink.hpp"
#include "wfd/parser/options.h"
#include "wfd/parser/setup.h"
#include "wfd/parser/pause.h"
#include "wfd/parser/play.h"
#include "wfd/parser/teardown.h"
#include "wfd/parser/audiocodecs.h"
#include "wfd/parser/videoformats.h"
#include "wfd/parser/formats3d.h"
#include "wfd/parser/clientrtpports.h"
#include "wfd/parser/presentationurl.h"
#include "wfd/parser/displayedid.h"
#include "wfd/parser/coupledsink.h"
#include "wfd/parser/i2c.h"
#include "wfd/parser/connectortype.h"
#include "wfd/parser/standbyresumecapability.h"


MiracSink::SetParameterType MiracSink::get_method(std::shared_ptr<WFD::SetParameter> set_param)
{
  auto method = set_param->payload().get_property (WFD::PropertyType::WFD_TRIGGER_METHOD);
  if (method) {
        auto trigger_method = std::static_pointer_cast<WFD::TriggerMethod> (method);
        switch (trigger_method->method()) {
            case WFD::TriggerMethod::SETUP:
                return M5_TRIGGER_SETUP;
            case WFD::TriggerMethod::PLAY:
                return M5_TRIGGER_PLAY;
            case WFD::TriggerMethod::PAUSE:
                return M5_TRIGGER_PAUSE;
            case WFD::TriggerMethod::TEARDOWN:
                return M5_TRIGGER_TEARDOWN;
            default:
                assert(false);
        }
    } else {
        return M4;
    }
}


void MiracSink::set_state(MiracSink::State state)
{
    state_ = state;
    if (state == INIT) {
        send_cseq_ = 1;
        gst_pipeline.reset(new MiracGstSink("", 0));
    }
    std::cout << "** State "<< state_ << std::endl;
}

void MiracSink::set_presentation_url (std::string url)
{
    presentation_url_ = url;
    std::cout << "** Got url "<< presentation_url_ << std::endl;
}

void MiracSink::set_session (std::string session)
{
    session_ = session;
    std::cout << "** Got session "<< session_ << std::endl;
}

void MiracSink::handle_m1_options (std::shared_ptr<WFD::Message> message)
{
    WFD::Reply reply(200);
    std::vector<WFD::Method> methods;

    set_state (CAPABILITY_NEGOTIATION);

    methods.push_back(WFD::Method::ORG_WFA_WFD_1_0);
    methods.push_back(WFD::Method::GET_PARAMETER);
    methods.push_back(WFD::Method::SET_PARAMETER);
    reply.header().set_supported_methods(methods);
    reply.header().set_cseq (message->header().cseq());

    send (reply);

    // Send M2 OPTIONS
    expected_reply_ = WFD::Method::OPTIONS;
    WFD::Options m2("*");
    m2.header().set_cseq (send_cseq_++);
    m2.header().set_require_wfd_support (true);
    send (m2);
}

void MiracSink::handle_m2_options_reply (std::shared_ptr<WFD::Reply> reply)
{
    // not expecting anything
    expected_reply_ = WFD::Method::ORG_WFA_WFD_1_0;

    // Ensure M2 OPTIONS reply is valid
    if (reply->response_code() != 200)
        return;

    auto methods = reply->header().supported_methods();

    if (std::find (methods.begin(), methods.end(), WFD::Method::ORG_WFA_WFD_1_0) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::GET_PARAMETER) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::SET_PARAMETER) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::SETUP) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::PLAY) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::TEARDOWN) == methods.end() ||
        std::find (methods.begin(), methods.end(), WFD::Method::PAUSE) == methods.end()) {

        std::cout << "M2 OPTIONS reply with insufficient methods" << std::endl;
    }

}

void MiracSink::handle_m3_get_parameter (std::shared_ptr<WFD::Message> message)
{
    WFD::Reply reply(200);
    reply.header().set_cseq (message->header().cseq());

    auto props = message->payload().get_parameter_properties();
    for (auto it = props.begin(); it != props.end(); it++) {
        std::shared_ptr<WFD::Property> new_prop;
        if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_AUDIO_CODECS]){
            // declare that we support absolutely everything, let gstreamer deal with it
            auto codec_lpcm = new WFD::AudioCodec (WFD::AudioFormat::LPCM, WFD::AudioFormat::Modes(3), 0);
            auto codec_aac = new WFD::AudioCodec (WFD::AudioFormat::AAC, WFD::AudioFormat::Modes(15), 0);
            auto codec_ac3 = new WFD::AudioCodec (WFD::AudioFormat::AC3, WFD::AudioFormat::Modes(7), 0);
            auto codec_list = std::vector<WFD::AudioCodec>();
            codec_list.push_back(*codec_lpcm);
            codec_list.push_back(*codec_aac);
            codec_list.push_back(*codec_ac3);
            new_prop.reset(new WFD::AudioCodecs(codec_list));
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_VIDEO_FORMATS]){
            auto codec_list = WFD::H264Codecs();
            // again, declare that we support absolutely everything, let gstreamer deal with it
            auto codec_cbp = new WFD::H264Codec(1, 16, 0x1ffff, 0x1fffffff, 0xfff, 0, 0, 0, 0x11, 0, 0);
            auto codec_chp = new WFD::H264Codec(2, 16, 0x1ffff, 0x1fffffff, 0xfff, 0, 0, 0, 0x11, 0, 0);
            codec_list.push_back(*codec_cbp);
            codec_list.push_back(*codec_chp);
            new_prop.reset(new WFD::VideoFormats(64 , 0, codec_list)); // 64 should mean 1920x1080p24
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_3D_FORMATS]){
            new_prop.reset(new WFD::Formats3d());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_CONTENT_PROTECTION]){
            new_prop.reset(new WFD::ContentProtection());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_DISPLAY_EDID]){
            new_prop.reset(new WFD::DisplayEdid());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_COUPLED_SINK]){
            new_prop.reset(new WFD::CoupledSink());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_CLIENT_RTP_PORTS]){
            new_prop.reset(new WFD::ClientRtpPorts(gst_pipeline->sink_udp_port(), 0));
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_I2C]){
            new_prop.reset(new WFD::I2C(0));
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_UIBC_CAPABILITY]){
            new_prop.reset(new WFD::UIBCCapability());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_CONNECTOR_TYPE]){
            new_prop.reset(new WFD::ConnectorType());
            reply.payload().add_property(new_prop);
        } else if (*it == WFD::PropertyName::name[WFD::PropertyType::WFD_STANDBY_RESUME_CAPABILITY]){
            new_prop.reset(new WFD::StandbyResumeCapability(false));
            reply.payload().add_property(new_prop);
        } else {
            std::cout << "** GET_PARAMETER: Property not supported" << std::endl;
        }
    }

    send (reply);
}

void MiracSink::handle_m4_set_parameter (std::shared_ptr<WFD::Message> message, bool initial)
{
    WFD::Reply reply(200);
    reply.header().set_cseq (message->header().cseq());

    auto props = message->payload().properties();

    // presentation URL is the only thing we care about
    // support for other parameters can be added later as needed
    if (initial) {
        auto prop = props.find (WFD::PropertyName::name[WFD::PropertyType::WFD_PRESENTATION_URL]);
        if (prop == props.end()) {
            reply.set_response_code (303);
            std::cout << "** SET_PARAMETER: missing wfd_presentation_url" << std::endl;
            // Is 404 the right code? The spec is unclear...
            std::shared_ptr<WFD::PropertyErrors> error(new WFD::PropertyErrors(WFD::PropertyType::WFD_PRESENTATION_URL, {404}));
            reply.payload().add_property_error(error);
        } else {
            auto url = std::static_pointer_cast<WFD::PresentationUrl>((*prop).second);
            set_presentation_url (url->presentation_url_1());
        }
    }

    if (reply.response_code() == 200)
        set_state(RTSP_SESSION_ESTABLISHMENT);

    send (reply);
}

void MiracSink::handle_m5_trigger (std::shared_ptr<WFD::Message> message,
                                   TriggeredCommand command)
{
    WFD::Reply reply(200);
    reply.header().set_cseq (message->header().cseq());

    send (reply);

    (this->*command)();
}

void MiracSink::handle_m6_setup_reply (std::shared_ptr<WFD::Reply> reply)
{
    // not expecting anything
    expected_reply_ = WFD::Method::ORG_WFA_WFD_1_0;

    // Ensure M6 SETUP reply is valid
    if (reply->response_code() != 200)
        return;

    if (reply->header().session().empty()) {
        std::cout << "** M6 reply without a session id, dropping" << std::endl;
        return;
    }
    set_session (reply->header().session());

    set_state (WFD_SESSION_ESTABLISHMENT);

    Play();
}

void MiracSink::handle_m7_play_reply (std::shared_ptr<WFD::Reply> reply)
{
    // not expecting anything
    expected_reply_ = WFD::Method::ORG_WFA_WFD_1_0;

    // Ensure M7 PLAY reply is valid
    if (reply->response_code() != 200)
        return;

    set_state (WFD_SESSION_PLAYING);

    // UDP packets should start flowing into gstreamer pipeline...
}

void MiracSink::handle_m8_teardown_reply (std::shared_ptr<WFD::Reply> reply)
{
    // not expecting anything
    expected_reply_ = WFD::Method::ORG_WFA_WFD_1_0;

    // Ensure M8 TEARDOWN reply is valid
    if (reply->response_code() != 200)
        return;

    set_state (INIT);
}

void MiracSink::handle_m9_pause_reply (std::shared_ptr<WFD::Reply> reply)
{
    // not expecting anything
    expected_reply_ = WFD::Method::ORG_WFA_WFD_1_0;

    // Ensure M9 PAUSE reply is valid
    if (reply->response_code() != 200)
      return;

    set_state (WFD_SESSION_PAUSED);
}

bool MiracSink::validate_message_sequence(std::shared_ptr<WFD::Message> message) const
{
    if (message->type() == WFD::Message::MessageTypeReply) {
        // Ensure sent message sequence
        if (message->header().cseq() != send_cseq_ - 1) {
            std::cout << "** Out-of-order reply CSeq, dropping reply" << std::endl;
            return false;
        }
        if (expected_reply_ == WFD::Method::ORG_WFA_WFD_1_0) {
            std::cout << "** Not expecting reply, dropping reply" << std::endl;
            return false;
        }
    } else {
        // Ensure received message sequence
        if (message->type() != WFD::Message::MessageTypeOptions &&
            message->header().cseq() != receive_cseq_ + 1) {

            WFD::Reply reply(400);
            reply.header().set_cseq (message->header().cseq());
            send (reply);

            std::cout << "** Out-of-order message CSeq " << message->header().cseq() << std::endl;
            return false;
        }

        if (expected_reply_ != WFD::Method::ORG_WFA_WFD_1_0) {
            WFD::Reply reply(400);
            reply.header().set_cseq (message->header().cseq());
            send (reply);

            std::cout << "** Expected a reply";
            return false;
        }
    }

    return true;
}

void MiracSink::got_message(std::shared_ptr<WFD::Message> message)
{
    std::cout << "** got msg: "  << std::endl << message->to_string() << std::endl;

    if (!validate_message_sequence (message))
        return;

    if (message->type() == WFD::Message::MessageTypeOptions)
        receive_cseq_ = message->header().cseq();
    else if (message->type() != WFD::Message::MessageTypeReply)
        receive_cseq_++;

    switch (message->type()) {
        case WFD::Message::MessageTypeOptions:
            if (state_ == INIT)
                handle_m1_options(message);
            else 
                std::cout << "** Unexpected OPTIONS" << std::endl;
            break;
        case WFD::Message::MessageTypeGetParameter:
            if (state_ >= CAPABILITY_NEGOTIATION)
                handle_m3_get_parameter(message);
            else 
                std::cout << "** Unexpected GET_PARAMETER" << std::endl;
            break;
        case WFD::Message::MessageTypeSetParameter:
        {
            auto set_param = std::static_pointer_cast<WFD::SetParameter>(message);
            if (state_ == CAPABILITY_NEGOTIATION &&
                get_method(set_param) == SetParameterType::M4) {
                handle_m4_set_parameter (message, true);
            } else if (state_ >= RTSP_SESSION_ESTABLISHMENT &&
                get_method(set_param) == SetParameterType::M4) {
                handle_m4_set_parameter (message, false);
            } else if (state_ == RTSP_SESSION_ESTABLISHMENT &&
                       get_method(set_param) == SetParameterType::M5_TRIGGER_SETUP) {
                handle_m5_trigger(message, &MiracSink::Setup);
            } else if ((state_ == WFD_SESSION_PLAYING ||
                        state_ == WFD_SESSION_PAUSED) &&
                       get_method(set_param) == SetParameterType::M5_TRIGGER_TEARDOWN) {
                handle_m5_trigger (message, &MiracSink::Teardown);
            } else if (state_ == WFD_SESSION_PLAYING &&
                       get_method(set_param) == SetParameterType::M5_TRIGGER_PAUSE) {
                handle_m5_trigger(message, &MiracSink::Pause);
            } else if (state_ == WFD_SESSION_PAUSED &&
                       get_method(set_param) == SetParameterType::M5_TRIGGER_PLAY) {
                handle_m5_trigger(message, &MiracSink::Play);
            } else {
                std::cout << "** Unexpected SET_PARAMETER" << std::endl;
            }
            break;
        }
        case WFD::Message::MessageTypeReply:
            if (state_ == CAPABILITY_NEGOTIATION &&
                expected_reply_ == WFD::Method::OPTIONS) {
                handle_m2_options_reply(std::static_pointer_cast<WFD::Reply>(message));
            } else if (state_ == RTSP_SESSION_ESTABLISHMENT &&
                       expected_reply_ == WFD::Method::SETUP) {
                handle_m6_setup_reply(std::static_pointer_cast<WFD::Reply>(message));
            } else if (state_ == WFD_SESSION_ESTABLISHMENT &&
                       expected_reply_ == WFD::Method::PLAY) {
                handle_m7_play_reply(std::static_pointer_cast<WFD::Reply>(message));
            } else if (state_ == WFD_SESSION_PLAYING) {
                switch (expected_reply_) {
                case WFD::Method::TEARDOWN:
                    handle_m8_teardown_reply(std::static_pointer_cast<WFD::Reply>(message));
                    break;
                case WFD::Method::PAUSE:
                    handle_m9_pause_reply(std::static_pointer_cast<WFD::Reply>(message));
                    break;
                default:
                    break;
                }
            } else if (state_ == WFD_SESSION_PAUSED) {
                switch (expected_reply_) {
                case WFD::Method::TEARDOWN:
                    handle_m8_teardown_reply(std::static_pointer_cast<WFD::Reply>(message));
                    break;
                case WFD::Method::PLAY:
                    handle_m7_play_reply(std::static_pointer_cast<WFD::Reply>(message));
                    break;
                default:
                    break;
                }
            } else {
                std::cout << "** Unexpected reply" << std::endl;
            }
            break;
        default:
            std::cout << "** Method not implemented" << std::endl;
            break;
    }
}

void MiracSink::on_connected()
{
    set_state(INIT);
}

MiracSink::MiracSink(const std::string& host, int rtsp_port)
    : MiracBroker(host.c_str(), std::to_string(rtsp_port)),
      send_cseq_(0),
      receive_cseq_(0) {

}

MiracSink::~MiracSink()
{
}

void MiracSink::Teardown() {
    std::cout << "** teardown" << std::endl;
    expected_reply_ = WFD::Method::TEARDOWN;
    WFD::Teardown m8(presentation_url_);
    m8.header().set_cseq (send_cseq_++);
    send(m8);
}

void MiracSink::Play() {
    std::cout << "** play" << std::endl;
    expected_reply_ = WFD::Method::PLAY;
    WFD::Play m7(presentation_url_);
    m7.header().set_session (session_);
    m7.header().set_cseq (send_cseq_++);
    send (m7);
}

void MiracSink::Pause() {
    std::cout << "** pause" << std::endl;
    expected_reply_ = WFD::Method::PAUSE;
    WFD::Pause m9(presentation_url_);
    m9.header().set_cseq (send_cseq_++);
    send(m9);
}

void MiracSink::Setup() {
    std::cout << "** setup" << std::endl;
    expected_reply_ = WFD::Method::SETUP;
    WFD::Setup m6(presentation_url_);
    auto transport = new WFD::TransportHeader();
    transport->set_client_port(gst_pipeline->sink_udp_port());
    m6.header().set_transport(transport);
    m6.header().set_cseq (send_cseq_++);
    send (m6);
}

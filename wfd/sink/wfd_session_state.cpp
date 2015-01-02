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

#include "wfd_session_state.h"

#include "wfd/public/media_manager.h"

#include "cap_negotiation_state.h"
#include "streaming_state.h"
#include "wfd/parser/play.h"
#include "wfd/parser/reply.h"
#include "wfd/parser/setup.h"
#include "wfd/parser/transportheader.h"

namespace wfd {
namespace sink {

class M6Handler final : public SequencedMessageSender {
 public:
    using SequencedMessageSender::SequencedMessageSender;
 private:
  virtual std::unique_ptr<Message> CreateMessage() override {
    auto setup = new Setup(ToSinkMediaManager(manager_)->GetPresentationUrl());
    auto transport = new TransportHeader();
    // we assume here that there is no coupled secondary sink
    transport->set_client_port(manager_->SinkRtpPorts().first);

    setup->header().set_transport(transport);
    setup->header().set_cseq(send_cseq_++);
    setup->header().set_require_wfd_support(true);

    return std::unique_ptr<Message>(setup);
  }

  virtual bool HandleReply(Reply* reply) override {
    const std::string& session_id = reply->header().session();
    if(reply->response_code() == 200 && !session_id.empty()) {
      ToSinkMediaManager(manager_)->SetSession(session_id);
      return true;
    }

    return false;
  }
};

class M7Handler final : public SequencedMessageSender {
 public:
    using SequencedMessageSender::SequencedMessageSender;
 private:
  virtual std::unique_ptr<Message> CreateMessage() override {
    Play* play = new Play(ToSinkMediaManager(manager_)->GetPresentationUrl());
    play->header().set_session(ToSinkMediaManager(manager_)->GetSession());
    play->header().set_cseq(send_cseq_++);
    play->header().set_require_wfd_support(true);

    return std::unique_ptr<Message>(play);
  }

  virtual bool HandleReply(Reply* reply) override {
    return (reply->response_code() == 200);
  }
};

WfdSessionState::WfdSessionState(const InitParams& init_params)
  : MessageSequenceWithOptionalSetHandler(init_params) {
  AddSequencedHandler(new M6Handler(init_params));
  AddSequencedHandler(new M7Handler(init_params));

  AddOptionalHandler(new M3Handler(init_params));
  AddOptionalHandler(new M4Handler(init_params));
  AddOptionalHandler(new TeardownHandler(init_params));
}

WfdSessionState::~WfdSessionState() {
}

}  // namespace sink
}  // namespace wfd

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

#include "streaming_state.h"

#include "context_manager.h"

#include "cap_negotiation_state.h"
#include "wfd_session_state.h"
#include "reply.h"
#include "typed_message.h"

namespace wfd {

class M9Handler final : public MessageReceiver<TypedMessage::M9> {
 public:
  M9Handler(ContextManager* manager, Observer* observer)
    : MessageReceiver<TypedMessage::M9>(manager, observer) {
  }

  virtual bool HandleMessage(std::unique_ptr<TypedMessage> message) override {
    if (manager_->IsPaused()) {
      assert(observer_);
      observer_->OnError(this);
      return false;
    }

    auto reply = std::unique_ptr<WFD::Reply>(new WFD::Reply(200));
    reply->header().set_cseq(message->cseq());
    manager_->Pause();
    manager_->SendRtspMessage(reply->to_string());
    return true;
  }
};

StreamingState::StreamingState(ContextManager* manager,
    MessageHandler::Observer* observer)
  : MessageSequenceWithOptionalSetHandler(manager, observer) {
  AddSequencedHandler(new M8Handler(manager, this));

  AddOptionalHandler(new M7Handler(manager, this));
  AddOptionalHandler(new M9Handler(manager, this));
}

StreamingState::~StreamingState() {
}

}  // miracast

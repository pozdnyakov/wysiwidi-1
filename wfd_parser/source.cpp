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

#include "source.h"

#include <algorithm>

#include "driver.h"
#include "init_state.h"
#include "cap_negotiation_state.h"
#include "message_handler.h"
#include "streaming_state.h"
#include "wfd_session_state.h"
#include "typed_message.h"

namespace wfd {

namespace {

std::unique_ptr<TypedMessage> CreateTypedMessage(WFD::MessagePtr message) {
  switch(message->type()) {
  case WFD::Message::MessageTypeReply:
    return std::unique_ptr<TypedMessage>(new Reply(message));
  case WFD::Message::MessageTypeOptions:
    return std::unique_ptr<TypedMessage>(new M2(message));
  case WFD::Message::MessageTypeSetup:
    return std::unique_ptr<TypedMessage>(new M6(message));
  case WFD::Message::MessageTypePlay:
    return std::unique_ptr<TypedMessage>(new M7(message));
  case WFD::Message::MessageTypeTeardown:
    return std::unique_ptr<TypedMessage>(new M8(message));
  case WFD::Message::MessageTypePause:
    return std::unique_ptr<TypedMessage>(new M9(message));
  case WFD::Message::MessageTypeSetParameter:
    if (message->payload().has_property(WFD::WFD_ROUTE))
      return std::unique_ptr<TypedMessage>(new M10(message));
    else if (message->payload().has_property(WFD::WFD_CONNECTOR_TYPE))
      return std::unique_ptr<TypedMessage>(new M11(message));
    else if (message->payload().has_property(WFD::WFD_STANDBY))
      return std::unique_ptr<TypedMessage>(new M12(message));
    else if (message->payload().has_property(WFD::WFD_IDR_REQUEST))
      return std::unique_ptr<TypedMessage>(new M13(message));
    else if (message->payload().has_property(WFD::WFD_UIBC_CAPABILITY))
      return std::unique_ptr<TypedMessage>(new M14(message));
    else if (message->payload().has_property(WFD::WFD_UIBC_SETTING))
      return std::unique_ptr<TypedMessage>(new M15(message));
  default:
      break;
      // TODO: warning.
  }

  return nullptr;
}

std::vector<MessageHandler*> CreateStates(
  ContextManager* mng, MessageHandler::Observer* observer) {
  std::vector<MessageHandler*> states;
  states.push_back(new InitState(mng, observer));
  states.push_back(new CapNegotiationState(mng, observer));
  states.push_back(new WfdSessionState(mng, observer));
  states.push_back(new StreamingState(mng, observer));
  return states;
}

}
class StateMachine : public MessageSequenceHandler{
 public:
   StateMachine(ContextManager* mng,
                MessageHandler::Observer* observer)
     : MessageSequenceHandler(mng, observer) {
     AddSequencedHandler(new InitState(mng, this));
     AddSequencedHandler(new CapNegotiationState(mng, this));
     AddSequencedHandler(new WfdSessionState(mng, this));
     AddSequencedHandler(new StreamingState(mng, this));
   }
};

class SourceImpl : public Source, public MessageHandler::Observer
{
 public:
  explicit SourceImpl(ContextManager* mng);
  virtual ~SourceImpl();

 private:
  // Source implementation.
  virtual void Start() override;
  virtual void RtspMessageReceived(const std::string& message) override;

  // MessageHandler::Observer implementation.
  virtual void OnCompleted(MessageHandler* handler) override;
  virtual void OnError(MessageHandler* handler) override;

  bool GetHeader(std::string& header);
  bool GetPayload(std::string& payload, unsigned content_length);

  std::unique_ptr<MessageSequenceHandler> state_machine_;
  WFD::Driver driver_;
  std::string rtsp_recieve_buffer_;
};

Source* Source::Create(ContextManager* mng) {
  return new SourceImpl(mng);
}

SourceImpl::SourceImpl(ContextManager* mng)
  : state_machine_(new StateMachine(mng, this)) {
}

SourceImpl::~SourceImpl() {
}

void SourceImpl::Start() {
  state_machine_->Start();
}

void SourceImpl::RtspMessageReceived(const std::string& message) {
  rtsp_recieve_buffer_ += message;
  std::string buffer;

  while(GetHeader(buffer)) {
    driver_.parse_header(buffer);
    WFD::MessagePtr rtsp_message = driver_.parsed_message();
    if (!rtsp_message.get()) {
      // TODO : handle an invalid input.
      rtsp_recieve_buffer_.clear();
      return;
    }
    uint content_length = rtsp_message->header().content_length();
    if (content_length && GetPayload(buffer, content_length))
      driver_.parse_payload(buffer);
      auto typed_message = CreateTypedMessage(rtsp_message);
      if (typed_message)
        state_machine_->Handle(std::move(typed_message));
  }
}

bool SourceImpl::GetHeader(std::string& header) {
  size_t eom = rtsp_recieve_buffer_.find("\r\n\r\n");
  if (eom == std::string::npos) {
    rtsp_recieve_buffer_.clear();
    return false;
  }

  header = rtsp_recieve_buffer_.substr(0, eom + 4);
  rtsp_recieve_buffer_.erase(0, eom + 4);
  return true;
}

bool SourceImpl::GetPayload(std::string& payload, unsigned content_length) {
  if (rtsp_recieve_buffer_.size() < content_length)
      return false;

  payload = rtsp_recieve_buffer_.substr(0, content_length);
  rtsp_recieve_buffer_.erase(0, content_length);
  return true;
}

void SourceImpl::OnCompleted(MessageHandler* handler) {}
void SourceImpl::OnError(MessageHandler* handler) {}

}  // namespace wfd

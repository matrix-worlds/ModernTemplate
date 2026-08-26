/* -*- C++ -*- */
//
// SipMessage.h
//
// SIP (RFC 3261) request/response messages built on Message.h. Headers
// are reduced to what a transaction needs to route messages (Call-ID,
// Via branch, CSeq) -- this is not a wire-format parser.
//
// SipRequestT<Method> and SipResponseT<Class> are non-type-template
// parameterizations: each concrete SIP method (INVITE, BYE, ...) and
// each response class (1xx/2xx/3xx/4xx/5xx/6xx) is its own distinct C++
// type, sharing 100% of its implementation with every other instantiation.
// The payoff: Protocol.h's ProtocolStateMachine dispatches by
// std::type_index, so "handle an INVITE" vs "handle a BYE", or "handle a
// 2xx" vs "handle a 3xx-6xx", can be separate table entries without any
// runtime method/status-code branching inside a handler -- see
// SipTransaction.h. A response's exact status code is still a runtime
// value (180 vs 183 are both SipResponseT<Provisional>); the template
// parameter only fixes which *class* it belongs to, and the constructor
// throws std::invalid_argument if a caller passes a status code that
// doesn't match the class it's being constructed as.

#ifndef MODERNTEMPLATE_SIPMESSAGE_H
#define MODERNTEMPLATE_SIPMESSAGE_H

#include "Message.h"
#include <stdexcept>
#include <string>

namespace ModernCommon {

  enum class SipMethod { Invite, Ack, Bye, Cancel, Register, Options };

  inline std::string toString(SipMethod m)
  {
    switch (m)
    {
      case SipMethod::Invite:   return "INVITE";
      case SipMethod::Ack:      return "ACK";
      case SipMethod::Bye:      return "BYE";
      case SipMethod::Cancel:   return "CANCEL";
      case SipMethod::Register: return "REGISTER";
      case SipMethod::Options:  return "OPTIONS";
    }
    throw std::invalid_argument("unknown SipMethod");
  }

  /// Fields common to every SIP request/response (RFC 3261 SS8, SS20),
  /// reduced to what's needed to correlate a message to a transaction.
  class SipMessage : public Message
  {
  public:
    const std::string& getCallId() const { return _callId; }
    const std::string& getViaBranch() const { return _viaBranch; }
    int getCSeqNumber() const { return _cseqNumber; }
    SipMethod getCSeqMethod() const { return _cseqMethod; }

  protected:
    SipMessage(std::string callId, std::string viaBranch, int cseqNumber, SipMethod cseqMethod)
      : _callId(std::move(callId)), _viaBranch(std::move(viaBranch)),
        _cseqNumber(cseqNumber), _cseqMethod(cseqMethod)
    {}

  private:
    std::string _callId;
    std::string _viaBranch;
    int _cseqNumber;
    SipMethod _cseqMethod;
  };

  // ---- Requests -----------------------------------------------------

  class SipRequest : public SipMessage
  {
  public:
    SipMethod getMethod() const { return _method; }
    std::string getName() const override { return toString(_method); }
    const std::string& getRequestUri() const { return _requestUri; }

  protected:
    SipRequest(SipMethod method, std::string requestUri, std::string callId,
               std::string viaBranch, int cseqNumber)
      : SipMessage(std::move(callId), std::move(viaBranch), cseqNumber, method),
        _method(method), _requestUri(std::move(requestUri))
    {}

  private:
    SipMethod _method;
    std::string _requestUri;
  };

  template<SipMethod M>
  class SipRequestT : public SipRequest
  {
  public:
    SipRequestT(std::string requestUri, std::string callId, std::string viaBranch, int cseqNumber)
      : SipRequest(M, std::move(requestUri), std::move(callId), std::move(viaBranch), cseqNumber)
    {}
  };

  using InviteRequest   = SipRequestT<SipMethod::Invite>;
  using AckRequest       = SipRequestT<SipMethod::Ack>;
  using ByeRequest        = SipRequestT<SipMethod::Bye>;
  using CancelRequest     = SipRequestT<SipMethod::Cancel>;
  using RegisterRequest   = SipRequestT<SipMethod::Register>;
  using OptionsRequest    = SipRequestT<SipMethod::Options>;

  // ---- Responses ------------------------------------------------------

  enum class SipResponseClass { Provisional, Success, Redirection, ClientError, ServerError, GlobalFailure };

  constexpr SipResponseClass classifySipStatusCode(int code)
  {
    if (code >= 100 && code < 200) return SipResponseClass::Provisional;
    if (code >= 200 && code < 300) return SipResponseClass::Success;
    if (code >= 300 && code < 400) return SipResponseClass::Redirection;
    if (code >= 400 && code < 500) return SipResponseClass::ClientError;
    if (code >= 500 && code < 600) return SipResponseClass::ServerError;
    if (code >= 600 && code < 700) return SipResponseClass::GlobalFailure;
    throw std::invalid_argument("SIP status code out of range: " + std::to_string(code));
  }

  class SipResponse : public SipMessage
  {
  public:
    int getStatusCode() const { return _statusCode; }
    const std::string& getReasonPhrase() const { return _reasonPhrase; }
    SipResponseClass getResponseClass() const { return _responseClass; }
    bool isProvisional() const { return _responseClass == SipResponseClass::Provisional; }
    bool isSuccess() const { return _responseClass == SipResponseClass::Success; }
    bool isFinal() const { return !isProvisional(); }

    std::string getName() const override { return std::to_string(_statusCode) + " " + _reasonPhrase; }

  protected:
    SipResponse(int statusCode, std::string reasonPhrase, std::string callId,
                std::string viaBranch, int cseqNumber, SipMethod cseqMethod)
      : SipMessage(std::move(callId), std::move(viaBranch), cseqNumber, cseqMethod),
        _statusCode(statusCode), _reasonPhrase(std::move(reasonPhrase)),
        _responseClass(classifySipStatusCode(statusCode))
    {}

  private:
    int _statusCode;
    std::string _reasonPhrase;
    SipResponseClass _responseClass;
  };

  template<SipResponseClass C>
  class SipResponseT : public SipResponse
  {
  public:
    SipResponseT(int statusCode, std::string reasonPhrase, std::string callId,
                 std::string viaBranch, int cseqNumber, SipMethod cseqMethod)
      : SipResponse(statusCode, std::move(reasonPhrase), std::move(callId),
                    std::move(viaBranch), cseqNumber, cseqMethod)
    {
      if (getResponseClass() != C)
      {
        throw std::invalid_argument(
          "status code " + std::to_string(statusCode) + " does not belong to the requested SipResponseT<> class");
      }
    }
  };

  using ProvisionalResponse   = SipResponseT<SipResponseClass::Provisional>;
  using SuccessResponse       = SipResponseT<SipResponseClass::Success>;
  using RedirectionResponse   = SipResponseT<SipResponseClass::Redirection>;
  using ClientErrorResponse   = SipResponseT<SipResponseClass::ClientError>;
  using ServerErrorResponse   = SipResponseT<SipResponseClass::ServerError>;
  using GlobalFailureResponse = SipResponseT<SipResponseClass::GlobalFailure>;

} // namespace ModernCommon

#endif // MODERNTEMPLATE_SIPMESSAGE_H

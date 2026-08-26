/* -*- C++ -*- */
//
// test_SipMessage.cpp
//
// Exercises SipRequestT<Method>/SipResponseT<Class> -- see SipMessage.h
// for the design rationale (distinct C++ types per method/class,
// enabling Protocol.h's typeid-based dispatch).

#include "MiniTest.h"
#include "SipMessage.h"
#include <stdexcept>
#include <typeindex>

using namespace ModernCommon;

namespace {

  void test_requestFieldsAndMethod(MiniTest& t)
  {
    InviteRequest invite("sip:bob@example.com", "call-1", "z9hG4bK-1", 1);
    MT_CHECK(t, invite.getMethod() == SipMethod::Invite);
    MT_CHECK(t, invite.getName() == "INVITE");
    MT_CHECK(t, invite.getRequestUri() == "sip:bob@example.com");
    MT_CHECK(t, invite.getCallId() == "call-1");
    MT_CHECK(t, invite.getViaBranch() == "z9hG4bK-1");
    MT_CHECK(t, invite.getCSeqNumber() == 1);
    MT_CHECK(t, invite.getCSeqMethod() == SipMethod::Invite);
  }

  void test_differentMethodsAreDistinctTypes(MiniTest& t)
  {
    // The whole point of SipRequestT<Method>: Protocol.h dispatches by
    // std::type_index, so these must differ.
    MT_CHECK(t, std::type_index(typeid(InviteRequest)) != std::type_index(typeid(ByeRequest)));
    MT_CHECK(t, std::type_index(typeid(ByeRequest)) != std::type_index(typeid(CancelRequest)));
    MT_CHECK(t, std::type_index(typeid(InviteRequest)) == std::type_index(typeid(InviteRequest)));
  }

  void test_toStringCoversEveryMethod(MiniTest& t)
  {
    MT_CHECK(t, toString(SipMethod::Invite) == "INVITE");
    MT_CHECK(t, toString(SipMethod::Ack) == "ACK");
    MT_CHECK(t, toString(SipMethod::Bye) == "BYE");
    MT_CHECK(t, toString(SipMethod::Cancel) == "CANCEL");
    MT_CHECK(t, toString(SipMethod::Register) == "REGISTER");
    MT_CHECK(t, toString(SipMethod::Options) == "OPTIONS");
  }

  void test_classifySipStatusCodeBoundaries(MiniTest& t)
  {
    MT_CHECK(t, classifySipStatusCode(100) == SipResponseClass::Provisional);
    MT_CHECK(t, classifySipStatusCode(199) == SipResponseClass::Provisional);
    MT_CHECK(t, classifySipStatusCode(200) == SipResponseClass::Success);
    MT_CHECK(t, classifySipStatusCode(299) == SipResponseClass::Success);
    MT_CHECK(t, classifySipStatusCode(300) == SipResponseClass::Redirection);
    MT_CHECK(t, classifySipStatusCode(404) == SipResponseClass::ClientError);
    MT_CHECK(t, classifySipStatusCode(503) == SipResponseClass::ServerError);
    MT_CHECK(t, classifySipStatusCode(600) == SipResponseClass::GlobalFailure);
  }

  void test_classifySipStatusCodeOutOfRangeThrows(MiniTest& t)
  {
    bool threw = false;
    try { classifySipStatusCode(50); }
    catch (const std::invalid_argument&) { threw = true; }
    MT_CHECK(t, threw);

    threw = false;
    try { classifySipStatusCode(700); }
    catch (const std::invalid_argument&) { threw = true; }
    MT_CHECK(t, threw);
  }

  void test_responseConstructionWithMatchingClassSucceeds(MiniTest& t)
  {
    ProvisionalResponse ringing(180, "Ringing", "call-1", "z9hG4bK-1", 1, SipMethod::Invite);
    MT_CHECK(t, ringing.getStatusCode() == 180);
    MT_CHECK(t, ringing.getReasonPhrase() == "Ringing");
    MT_CHECK(t, ringing.getResponseClass() == SipResponseClass::Provisional);
    MT_CHECK(t, ringing.isProvisional());
    MT_CHECK(t, !ringing.isSuccess());
    MT_CHECK(t, !ringing.isFinal());
    MT_CHECK(t, ringing.getName() == "180 Ringing");
    MT_CHECK(t, ringing.getCSeqMethod() == SipMethod::Invite); // echoes the INVITE, not itself

    SuccessResponse ok(200, "OK", "call-1", "z9hG4bK-1", 1, SipMethod::Invite);
    MT_CHECK(t, ok.isSuccess());
    MT_CHECK(t, ok.isFinal());
  }

  /// The constructor validates its own class -- a 404 built as a
  /// SipResponseT<Success> is a construction-time error, not a bug
  /// waiting to surface later at dispatch time.
  void test_responseConstructionWithMismatchedClassThrows(MiniTest& t)
  {
    bool threw = false;
    try
    {
      SuccessResponse notReallySuccess(404, "Not Found", "call-1", "z9hG4bK-1", 1, SipMethod::Invite);
    }
    catch (const std::invalid_argument&)
    {
      threw = true;
    }
    MT_CHECK(t, threw);
  }

  void test_differentResponseClassesAreDistinctTypes(MiniTest& t)
  {
    MT_CHECK(t, std::type_index(typeid(ProvisionalResponse)) != std::type_index(typeid(SuccessResponse)));
    MT_CHECK(t, std::type_index(typeid(ClientErrorResponse)) != std::type_index(typeid(ServerErrorResponse)));
  }

} // namespace

int main()
{
  MiniTest t("test_SipMessage");
  MT_RUN(t, test_requestFieldsAndMethod);
  MT_RUN(t, test_differentMethodsAreDistinctTypes);
  MT_RUN(t, test_toStringCoversEveryMethod);
  MT_RUN(t, test_classifySipStatusCodeBoundaries);
  MT_RUN(t, test_classifySipStatusCodeOutOfRangeThrows);
  MT_RUN(t, test_responseConstructionWithMatchingClassSucceeds);
  MT_RUN(t, test_responseConstructionWithMismatchedClassThrows);
  MT_RUN(t, test_differentResponseClassesAreDistinctTypes);
  return t.result();
}

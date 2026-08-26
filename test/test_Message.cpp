/* -*- C++ -*- */
//
// test_Message.cpp
//
// Exercises Message/MessageHandle -- see Message.h for the design
// rationale (std::shared_ptr + std::dynamic_pointer_cast, no separate
// typesafe handle wrapper needed).

#include "MiniTest.h"
#include "Message.h"
#include <memory>

using namespace ModernCommon;

namespace {

  class PingMessage : public Message
  {
  public:
    explicit PingMessage(int sequence) : _sequence(sequence) {}
    std::string getName() const override { return "Ping"; }
    int getSequence() const { return _sequence; }

  private:
    int _sequence;
  };

  class PongMessage : public Message
  {
  public:
    std::string getName() const override { return "Pong"; }
  };

  void test_getName(MiniTest& t)
  {
    PingMessage msg(1);
    MT_CHECK(t, msg.getName() == "Ping");
  }

  void test_messageHandleOwnership(MiniTest& t)
  {
    MessageHandle h = std::make_shared<PingMessage>(7);
    MT_CHECK(t, h != nullptr);
    MT_CHECK(t, h->getName() == "Ping");
  }

  /// dynamic_pointer_cast<ConcreteMsg> is the typesafe downcast for a
  /// MessageHandle -- see the file comment.
  void test_dynamicPointerCastToConcreteTypeSucceeds(MiniTest& t)
  {
    MessageHandle h = std::make_shared<PingMessage>(42);

    std::shared_ptr<PingMessage> ping = std::dynamic_pointer_cast<PingMessage>(h);
    MT_CHECK(t, ping != nullptr);
    MT_CHECK(t, ping->getSequence() == 42);
  }

  void test_dynamicPointerCastToWrongTypeReturnsNull(MiniTest& t)
  {
    MessageHandle h = std::make_shared<PingMessage>(1);

    std::shared_ptr<PongMessage> pong = std::dynamic_pointer_cast<PongMessage>(h);
    MT_CHECK(t, pong == nullptr); // wrong concrete type -- no throw, just null
  }

  void test_handleCanBeCopiedAndSharesOwnership(MiniTest& t)
  {
    MessageHandle h1 = std::make_shared<PingMessage>(9);
    MessageHandle h2 = h1;

    MT_CHECK(t, h1.get() == h2.get());
    MT_CHECK(t, h1.use_count() == 2);
  }

} // namespace

int main()
{
  MiniTest t("test_Message");
  MT_RUN(t, test_getName);
  MT_RUN(t, test_messageHandleOwnership);
  MT_RUN(t, test_dynamicPointerCastToConcreteTypeSucceeds);
  MT_RUN(t, test_dynamicPointerCastToWrongTypeReturnsNull);
  MT_RUN(t, test_handleCanBeCopiedAndSharesOwnership);
  return t.result();
}

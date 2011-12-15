/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include "ReplicatingSubscription.h"
#include "Logging.h"
#include "qpid/broker/Queue.h"
#include "qpid/broker/SessionContext.h"
#include "qpid/broker/ConnectionState.h"
#include "qpid/framing/AMQFrame.h"
#include "qpid/framing/MessageTransferBody.h"
#include "qpid/log/Statement.h"
#include "ostream"

namespace qpid {
namespace ha {

using namespace framing;
using namespace broker;
using namespace std;

const string ReplicatingSubscription::QPID_REPLICATING_SUBSCRIPTION("qpid.replicating-subscription");

namespace {
const string DOLLAR("$");
const string INTERNAL("-internal");
} // namespace

string mask(const string& in)
{
    return DOLLAR + in + INTERNAL;
}

boost::shared_ptr<broker::SemanticState::ConsumerImpl>
ReplicatingSubscription::Factory::create(
    SemanticState* parent,
    const string& name,
    Queue::shared_ptr queue,
    bool ack,
    bool /*acquire*/,
    bool exclusive,
    const string& tag,
    const string& resumeId,
    uint64_t resumeTtl,
    const framing::FieldTable& arguments
) {
    boost::shared_ptr<ReplicatingSubscription> rs;
    if (arguments.isSet(QPID_REPLICATING_SUBSCRIPTION)) {
        // FIXME aconway 2011-12-01: ignoring acquire param and setting acquire
        // false. Should this be done in the caller? Remove from ctor parameters.
        rs.reset(new ReplicatingSubscription(
                     parent, name, queue, ack, false, exclusive, tag,
                     resumeId, resumeTtl, arguments));
        queue->addObserver(rs);
    }
    return rs;
}

ReplicatingSubscription::ReplicatingSubscription(
    SemanticState* parent,
    const string& name,
    Queue::shared_ptr queue,
    bool ack,
    bool acquire,
    bool exclusive,
    const string& tag,
    const string& resumeId,
    uint64_t resumeTtl,
    const framing::FieldTable& arguments
) : ConsumerImpl(parent, name, queue, ack, acquire, exclusive, tag,
                 resumeId, resumeTtl, arguments),
    events(new Queue(mask(name))),
    consumer(new DelegatingConsumer(*this))
{
    // FIXME aconway 2011-12-09: Failover optimization removed.
    // There was code here to re-use messages already on the backup
    // during fail-over. This optimization was removed to simplify
    // the logic till we get the basic replication stable, it
    // can be re-introduced later. Last revision with the optimization:
    // r1213258 | QPID-3603: Fix QueueReplicator subscription parameters.

    QPID_LOG(debug, "HA: Started " << *this << " subscription " << name);

    // Note that broker::Queue::getPosition() returns the sequence
    // number that will be assigned to the next message *minus 1*.

    // this->backupPosition tracks the position of the remote backup
    // queue, i.e. the sequence number for the next delivered message
    // *minus one*
    backupPosition = 0;

    // FIXME aconway 2011-12-15: ConsumerImpl::position is left at 0
    // so we will start consuming from the lowest numbered message.
    // This is incorrect if the sequence number wraps around, but
    // this is what all consumers currently do.
}

// Message is delivered in the subscription's connection thread.
bool ReplicatingSubscription::deliver(QueuedMessage& m) {
    // Add position events for the subscribed queue, not for the internal event queue.
    if (m.queue && m.queue == getQueue().get()) {
        assert(position == m.position);
        {
             sys::Mutex::ScopedLock l(lock);
             // this->position is the new position after enqueueing m locally.
             // this->backupPosition is the backup position before enqueueing m. 
             assert(position > backupPosition);
             if (position - backupPosition > 1) {
                 // Position has advanced because of messages dequeued ahead of us.
                 SequenceNumber send(position);
                 // Send the position before m was enqueued.
                 sendPositionEvent(--send, l); 
             }
             backupPosition = position;
        }
        QPID_LOG(trace, "HA: Replicating " << QueuePos(m) << " to " << *this);
    }
    return ConsumerImpl::deliver(m);
}

void ReplicatingSubscription::cancel()
{
    QPID_LOG(debug, "HA: Cancelled " << *this);
    getQueue()->removeObserver(boost::dynamic_pointer_cast<QueueObserver>(shared_from_this()));
}

ReplicatingSubscription::~ReplicatingSubscription() {}

//called before we get notified of the message being available and
//under the message lock in the queue
void ReplicatingSubscription::enqueued(const QueuedMessage& m)
{
    //delay completion
    m.payload->getIngressCompletion().startCompleter();
}

// Called with lock held.
void ReplicatingSubscription::sendDequeueEvent(const sys::Mutex::ScopedLock& l)
{
    QPID_LOG(trace, "HA: Sending dequeues " << dequeues << " to " << *this);
    string buf(dequeues.encodedSize(),'\0');
    framing::Buffer buffer(&buf[0], buf.size());
    dequeues.encode(buffer);
    dequeues.clear();
    buffer.reset();
    sendEvent(QueueReplicator::DEQUEUE_EVENT_KEY, buffer, l);
}

// Called with lock held.
void ReplicatingSubscription::sendPositionEvent(
    SequenceNumber position, const sys::Mutex::ScopedLock&l )
{
    QPID_LOG(trace, "HA: Sending position " << QueuePos(getQueue().get(), position)
             << " on " << *this);
    string buf(backupPosition.encodedSize(),'\0');
    framing::Buffer buffer(&buf[0], buf.size());
    position.encode(buffer);
    buffer.reset();
    sendEvent(QueueReplicator::POSITION_EVENT_KEY, buffer, l);
}

void ReplicatingSubscription::sendEvent(const std::string& key, framing::Buffer& buffer,
                                        const sys::Mutex::ScopedLock&)
{
    //generate event message
    boost::intrusive_ptr<Message> event = new Message();
    AMQFrame method((MessageTransferBody(ProtocolVersion(), string(), 0, 0)));
    AMQFrame header((AMQHeaderBody()));
    AMQFrame content((AMQContentBody()));
    content.castBody<AMQContentBody>()->decode(buffer, buffer.getSize());
    header.setBof(false);
    header.setEof(false);
    header.setBos(true);
    header.setEos(true);
    content.setBof(false);
    content.setEof(true);
    content.setBos(true);
    content.setEos(true);
    event->getFrames().append(method);
    event->getFrames().append(header);
    event->getFrames().append(content);

    DeliveryProperties* props = event->getFrames().getHeaders()->get<DeliveryProperties>(true);
    props->setRoutingKey(key);
    // Send the event using the events queue. Consumer is a
    // DelegatingConsumer that delegates to *this for everything but
    // has an independnet position. We put an event on events and
    // dispatch it through ourselves to send it in line with the
    // normal browsing messages.
    events->deliver(event);
    events->dispatch(consumer);
}

// Called after the message has been removed from the deque and under
// the message lock in the queue.
void ReplicatingSubscription::dequeued(const QueuedMessage& m)
{
    {
        sys::Mutex::ScopedLock l(lock);
        dequeues.add(m.position);
        QPID_LOG(trace, "HA: Will dequeue " << QueuePos(m) << " on " << *this);
    }
    notify();                   // Ensure a call to doDispatch
    if (m.position > position) {
        m.payload->getIngressCompletion().finishCompleter();
        QPID_LOG(trace, "HA: Completed " << QueuePos(m) << " early on " << *this);
    }
}

bool ReplicatingSubscription::doDispatch()
{
    {
        sys::Mutex::ScopedLock l(lock);
        if (!dequeues.empty()) sendDequeueEvent(l);
    }
    return ConsumerImpl::doDispatch();
}

ReplicatingSubscription::DelegatingConsumer::DelegatingConsumer(ReplicatingSubscription& c) : Consumer(c.getName(), true), delegate(c) {}
ReplicatingSubscription::DelegatingConsumer::~DelegatingConsumer() {}
bool ReplicatingSubscription::DelegatingConsumer::deliver(QueuedMessage& m)
{
    return delegate.deliver(m);
}
void ReplicatingSubscription::DelegatingConsumer::notify() { delegate.notify(); }
bool ReplicatingSubscription::DelegatingConsumer::filter(boost::intrusive_ptr<Message> msg) { return delegate.filter(msg); }
bool ReplicatingSubscription::DelegatingConsumer::accept(boost::intrusive_ptr<Message> msg) { return delegate.accept(msg); }
OwnershipToken* ReplicatingSubscription::DelegatingConsumer::getSession() { return delegate.getSession(); }


ostream& operator<<(ostream& o, const ReplicatingSubscription& rs) {
    string url = rs.parent->getSession().getConnection().getUrl();
    return o << rs.getQueue()->getName() << " backup on " << url;
}

}} // namespace qpid::ha

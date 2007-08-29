/*
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

#include "SessionAdapter.h"

namespace qpid {
namespace broker {
using namespace framing;

SessionAdapter::SessionAdapter(Connection& c, ChannelId ch)
    : connection(c), channel(ch)
{
    // FIXME aconway 2007-08-29: When we handle session commands,
    // do this on open.
    session.reset(new Session(*this, 0));
}

SessionAdapter::~SessionAdapter() {}


void SessionAdapter::handle(AMQFrame& f) {
    // FIXME aconway 2007-08-29: handle session commands here, forward
    // other frames.
    session->in(f);
}


}} // namespace qpid::broker

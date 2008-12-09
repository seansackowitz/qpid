#ifndef QPID_SYS_ACTIVITYTIMER_H
#define QPID_SYS_ACTIVITYTIMER_H

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

#include "qpid/sys/Time.h"
#include "qpid/sys/Thread.h"
#include <boost/current_function.hpp>
#include <stdio.h>

namespace qpid {
namespace sys {

/** 
 * Measures and reports time spent in a particular segment of code.
 * This is real time so it includes time blocked/sleeping as well as time on CPU.
 *
 * Intended to be used via the QPID_ACTIVITY_TIMER macro for profiling
 * during development & debugging
 */
class ActivityTimer
{
  public:

    struct Stat {               // Must be a POD
        uint64_t total, count;
        void sample(uint64_t value) { total += value; ++count; }
        uint64_t mean() { return count ? total/count : 0; }
        void reset() { total = count = 0; }
    };

    struct Data {               // Must be a POD
        uint64_t start, entered, exited;
        Stat active, inactive;

        void reset() {
            start = entered = exited = 0;
            active.reset();
            inactive.reset();
        }

        void enter(uint64_t now) {
            entered=now;
            if (start) {
                assert(exited);
                assert(entered > exited);
                inactive.sample(entered - exited);
            }
            else
                start = Duration(now);
        }

        void exit(uint64_t now) {
            assert(start);
            assert(entered);
            exited=now;
            assert(exited > entered);
            active.sample(exited - entered);
        }
    };

    ActivityTimer(Data& d, const char* fn, const char* file, int line, uint64_t reportInterval) : data(d) {
        uint64_t now = Duration(qpid::sys::now());
        if (data.start && now - data.start > reportInterval)
            report(fn, file, line);
        data.enter(now);
    }

    ~ActivityTimer() {
        data.exit(Duration(now()));
    }

  private:
    Data& data;

    void report(const char* fn, const char* file, int line) {
        uint64_t secs = (Duration(now())-data.start)/TIME_SEC;
        printf("[%lu]%s:%d: %s: rate=%ld/sec active=%ld inactive=%ld\n",
               Thread::current().id(), file, line, fn,
               long(data.active.count/secs),
               long(data.active.mean()/TIME_USEC),
               long(data.inactive.mean()/TIME_USEC));
        data.reset();
    }
};

}} // namespace qpid::sys

/** Measures time between the point of declaration and the end of the innermost enclosing scope.
 * Can only have one in a given scope.
 */
#define ACTIVITY_TIMER(REPORT_INTERVAL_SECS) \
    static __thread ::qpid::sys::ActivityTimer::Data qpid__ActivityTimerData__ = { 0, 0, 0, { 0, 0 }, { 0, 0 } }; \
    ::qpid::sys::ActivityTimer qpid__ActivityTimerInstance__(qpid__ActivityTimerData__, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__, 2*::qpid::sys::TIME_SEC)

#endif  /*!QPID_SYS_ACTIVITYTIMER_H*/

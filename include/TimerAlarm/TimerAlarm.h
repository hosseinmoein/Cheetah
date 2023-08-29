// Hossein Moein
// August 29, 2023
/*
Copyright (c) 2023-2028, Hossein Moein
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Hossein Moein and/or the TimerAlarm nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Hossein Moein BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <condition_variable>
#include <mutex>

#include <cstdlib>
#include <ctime>
#include <iostream>

// ----------------------------------------------------------------------------

namespace hmta
{

// This timer is based on threads as oppose to the POSIX' signal based timers.
// F must define the operator()() which will be executed on either
// the engine_routine thread or on a separate thread depending on the value
// of "repeat_recursively".
//
template<typename F>
class   TimerAlarm  {

public:

    using time_type = time_t;
    using size_type = std::size_t;

    static const size_type  FOREVER_REPEATING = size_type(-1);

    TimerAlarm() = delete;
    TimerAlarm(const TimerAlarm &) = delete;
    TimerAlarm &operator = (const TimerAlarm &) = delete;

    // The timer is created in disarmed state.
    //
    TimerAlarm(F &functor,
               time_type interval_sec,
               time_type interval_nanosec = 0,

               // How many times do you want the timer to go off?
               //
               size_type repeat_count = FOREVER_REPEATING,

               // Do you want the timer to execute your functor at
               // each time interval, disregarding whether the
               // previous invocation of the functor had finished?
               //
               bool repeat_recursively = false);

    // It must wait for the engine_routine() to finish. It might wait for
    // at most one invocation of functor to finish, if the
    // repeat_recursively is false. Otherwise it will not wait for any
    // functor invocation.
    //
    ~TimerAlarm() noexcept;

    bool arm();     // It is _not_ OK (exception) to arm() an armed timer.
    bool disarm();  // It is OK to disarm() a disarmed timer.

    // The following method sets/changes the time interval. After a call
    // to the method, the time interval will change for the _next_ cycle.
    //
    bool set_time_interval(time_type interval_sec,
                           time_type interval_nanosec = 0);

    // There is no synchronization in the following two methods. They are
    // best estimates at the time.
    //
    inline bool is_armed() const noexcept;
    inline size_type current_repeat_count() const noexcept;

private:

    bool engine_routine_() noexcept;

    bool        is_armed_ { false };
    size_type   repeated_sofar_ { 0 };

    time_type       interval_sec_ { 0 };
    time_type       interval_nanosec_ { 0 };
    const size_type repeat_count_;
    const bool      repeat_recursively_;

    F   &functor_;

    mutable std::mutex      state_mutex_ { };
    std::condition_variable engine_cv_ { };
};

} // namespace hmta

// ----------------------------------------------------------------------------

#  ifndef HMTA_DO_NOT_INCLUDE_TCC_FILES
#    include <TimerAlarm/TimerAlarm.tcc>
#  endif // HMTA_DO_NOT_INCLUDE_TCC_FILES

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:

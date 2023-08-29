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

#include <TimerAlarm/TimerAlarm.h>

#include <chrono>
#include <stdexcept>
#include <thread>

// ----------------------------------------------------------------------------

namespace hmta
{

template<typename F>
TimerAlarm<F>::TimerAlarm(F &functor,
                          time_type interval_sec,
                          time_type interval_nanosec,
                          size_type repeat_count,
                          bool repeat_recursively)
    : is_armed_ (false),
      repeated_sofar_ (0),
      interval_sec_ (interval_sec),
      interval_nanosec_ (interval_nanosec),
      repeat_count_ (repeat_count),
      repeat_recursively_ (repeat_recursively),
      functor_ (functor)  {

    // Make sure everything is proper.
    //
    if (repeat_count_ == 0)
        throw std::runtime_error ("TimerAlarm::TimerAlarm(): "
                                  "repeat count must be greater then zero.");
    if (interval_sec_ <= 0 && interval_nanosec_ <= 0)
        throw std::runtime_error { "TimerAlarm::TimerAlarm(): "
                                   "the time interval must be greater then "
                                   "zero nano seconds." };
}

// ----------------------------------------------------------------------------

template<typename F>
TimerAlarm<F>::~TimerAlarm() noexcept  {

    std::unique_lock<std::mutex>    guard { state_mutex_ };

    if (is_armed_)  {
        is_armed_ = false;

        // Let the engine_routine() know it is time to quit.
        //
        engine_cv_.notify_one();

        // We must wait for the engine_routine() to finish.
        //
        engine_cv_.wait(guard);
    }
}

// ----------------------------------------------------------------------------

template<typename F>
bool TimerAlarm<F>::
set_time_interval(time_type interval_sec, time_type interval_nanosec)  {

    // Make sure everything is proper.
    //
    if (interval_sec <= 0 && interval_nanosec <= 0)
        throw std::runtime_error ("TimerAlarm::set_time_interval(): "
                                  "the time interval must be greater then "
                                  "zero nano seconds.");

    const std::lock_guard<std::mutex>   guard { state_mutex_ };

    interval_sec_ = interval_sec;
    interval_nanosec_ = interval_nanosec;

    return (true);
}

// ----------------------------------------------------------------------------

template<typename F>
bool TimerAlarm<F>::arm()  {

    const std::lock_guard<std::mutex>   guard { state_mutex_ };

    if (is_armed_)
        throw std::runtime_error { "TimerAlarm::arm(): "
                                   "The time/alarm is already armed." };

    std::thread engine_thr { &TimerAlarm::engine_routine_, this };

    engine_thr.detach();
    is_armed_ = true;
    repeated_sofar_ = 0;

    return (true);
}

// ----------------------------------------------------------------------------

template<typename F>
bool TimerAlarm<F>::disarm()  {

    const std::lock_guard<std::mutex>   guard { state_mutex_ };

    if (is_armed_)  {
        is_armed_ = false;

        // Let the engine_routine() know it is time to quit.
        //
        engine_cv_.notify_one();
        return (true);
    }

    return (false);
}

// ----------------------------------------------------------------------------

template<typename F>
bool TimerAlarm<F>::engine_routine_() noexcept  {

    size_type   this_count { repeat_count_ };

    while (this_count-- > 0)  {
        std::unique_lock<std::mutex>    guard { state_mutex_ };

        if (! is_armed_)
            break;

        const std::cv_status    signaled =
            engine_cv_.wait_for(
                guard,
                std::chrono::nanoseconds(
                    1000000000L * interval_sec_ + interval_nanosec_));

        // If we were signaled, it was a signal to disarm. So get out
        // of here immediately.
        //
        if (signaled == std::cv_status::no_timeout)
            break;

        repeated_sofar_ += 1;
        guard.release();
        state_mutex_.unlock();

        if (repeat_recursively_)  {
            std::thread functor_thr { &F::operator(), &functor_ };

            functor_thr.detach();
        }
        else
            functor_();
    }

    // In case we just run out of repeat count, disarm.
    //
    if (! disarm())

        // In case the destructor is waiting for us, signal it that
        // we are done.
        //
        engine_cv_.notify_one();

    return (true);
}

// ----------------------------------------------------------------------------

template<typename F>
bool TimerAlarm<F>::is_armed() const noexcept  { return (is_armed_); }

// ----------------------------------------------------------------------------

template<typename F>
typename TimerAlarm<F>::size_type TimerAlarm<F>::
current_repeat_count() const noexcept  { return (repeated_sofar_); }

} // namespace hmta

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:

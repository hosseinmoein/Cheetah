// Hossein Moein
// August 29, 2023

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

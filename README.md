<!--
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
-->
<img src="docs/Cheetah.jpg" alt="TimerAlarm Cheetah" width="400" longdesc="https://htmlpreview.github.io/?https://github.com/hosseinmoein/ThreadPool/blob/master/README.md"/>

This is a light-weight, header-only C++ Timer-alarm library that's based on threads as opposed to the POSIX' signal based timers. Therefore, it is a lot less disruptive.<BR>
It has the following logic:<BR>
1. It is templated with a class (functor) type. The functor (F) must define the `opperator()()`.
2. You instantiate an instance of TimeAlarm by calling the constructor and passing:
   1. A reference to an instance of the functor
   2. The second part of the time interval
   3. The nano-second part of the time interval. It is 0 by default
   4. The repeat count which specifies how many times the timer should repeat itself. It is set to `FOREVER_REPEATING` by default.
   5. A Boolean flag that specifies if you want to run the timer invocations recursively. A recursive invocation means the TimerAlarm calls you functor on time regardless of whether the previous invocation has finished. If true, this can potentially create a lot of threads depending on circumstances. By default, this flag is false.
3. You can `arm()` the TimerAlarm instance. That means the time-alarm will now be in effect. Once armed one thread will be created to run th timer, unless recursively flag is true which potentially can create more than one thread. The one thread is never destroyed and reused repeatedly. 
4. You can `disarm()` the TimerAlarm instance. That means the timer will no longer execute. The thread is destroyed at this time but not before it is finished.
5. You can always change the interval period by calling `set_time_interval()`.
6. You can query if the timer is armed by calling `is_armed()`.
7. You can query how many times the functor has been executed by calling `current_repeat_count()`.
8. The destructor waits until all timer invocations are done.

```cpp
class   MyFoot  {
public:

    MyFoot (int id) : id_ (id), count_ (0)  {  }

    bool operator ()()  {

        std::cout << "Printing from MyFoot (" << id_ << "). count is "
                  << count_ << ". time is " << time(nullptr) << std::endl;
        count_ += 1;
        return (true);
    }
    
private:

    int     id_;
    size_t  count_;
};

// ----------------------------------------------------------------------------

int main(int, char *[])  {

    const struct ::timespec rqt = { 60, 0 };

    {
        MyFoot              foot_master (10);
        // The functor foot_master must be executed every 5 seconds
        // "interval_nanosec" is set to zero by default.
        // "repeat_count" is set to forever by default
        // "repeat_recursively" is set to false by default
        //
        TimerAlarm<MyFoot>  timer (foot_master, 5);

        // The Timer will be armed
        //
        timer.arm();

        const struct ::timespec rqt2 = { 30, 0 };

        nanosleep(&rqt2, nullptr);
        
        // Change the interval to 1 seconds.
        // "interval_nanosec" is set to zero by default.
        //
        timer.set_time_interval(1);
        nanosleep(&rqt2, nullptr);
        
        // Change the interval to 10 seconds.
        // "interval_nanosec" is set to zero by default.
        //
        timer.set_time_interval(10);
        nanosleep(&rqt, nullptr);

        MyFoot              foot_master2 (200);
        // Construct a second timer with specifed parameters.
        //
        TimerAlarm<MyFoot>  timer2 (foot_master2,  // Functor instance
                                    5,  // 5 seconds intervals
                                    0,  // 0 nano-seconds specified
                                    TimerAlarm<MyFoot>::FOREVER_REPEATING,  // Repeat forever
                                    true);  // Repeat recursively

        timer2.arm();  // Armed timer will execute
        nanosleep(&rqt, nullptr);
    }

    std::cout << "\n\nmain(): Got out of the enclosing block ...\n" << std::endl;

    MyFoot             foot_master (3000);
    // Construct a third timer with specifed parameters.
    //
    TimerAlarm<MyFoot> timer (foot_master,  // Functor instance
                              5,  // 5 seconds intervals
                              0,  // 0 nano-seconds specified
                              5);  // Repeat 5 times

    timer.arm();  // Armed timer will execute
    nanosleep(&rqt, nullptr);
    return (EXIT_SUCCESS);
}
```

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

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace hmta;

// ----------------------------------------------------------------------------

class   MyFoot  {

    public:

        MyFoot (int id) : id_ (id), count_ (0)  {  }

        bool operator () ()  {

            std::cout << "Printing from MyFoot (" << id_
                      << "). count is " << count_
                      << ". time is " << time (nullptr) << std::endl;


            count_ += 1;
            return (true);
        }

    private:

        int     id_;
        size_t  count_;
};

// ----------------------------------------------------------------------------

int main(int, char *[])  {

    const struct ::timespec rqt = {60, 0};

    {
        MyFoot              foot_master (10);
        TimerAlarm<MyFoot>  timer (foot_master, 5);

        timer.arm ();

        const struct ::timespec rqt2 = {30, 0};

        nanosleep (&rqt2, nullptr);
        timer.set_time_interval (1);
        nanosleep (&rqt2, nullptr);
        timer.set_time_interval (10);
        nanosleep (&rqt, nullptr);

        // Repeat recursively.
        //
        MyFoot              foot_master2 (200);
        TimerAlarm<MyFoot>  timer2 (foot_master2,
                                    5,
                                    0,
                                    TimerAlarm<MyFoot>::FOREVER_REPEATING,
                                    true);

        timer2.arm ();

        nanosleep (&rqt, nullptr);
    }

    std::cout << std::endl << std::endl
              << "main(): Got out of the enclosing block ..."
              << std::endl << std::endl;

    // Go off only once.
    //
    MyFoot             foot_master (3000);
    TimerAlarm<MyFoot> timer (foot_master, 5, 0, 5);

    timer.arm ();

    nanosleep (&rqt, nullptr);

    return (EXIT_SUCCESS);
}

// ----------------------------------------------------------------------------

// Local Variables:
// mode:C++
// tab-width:4
// c-basic-offset:4
// End:

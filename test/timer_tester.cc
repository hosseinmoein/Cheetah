// Hossein Moein
// August 29, 2023

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

#include <time.h>
#include <stdlib.h>
 
#include <iostream>
 
long long getCurrentTimeMCS()
{
    timespec t = { 0 };
    clock_gettime( CLOCK_REALTIME, &t );
    return static_cast<long long>(t.tv_sec) * static_cast<long long>(1000000)
        + static_cast<long long>(t.tv_nsec) / 1000;
}
 
template<typename T>
struct sss
{
    volatile T t1_;
    volatile T t2_;
};
 
template<typename T>
long long do_loop( T number_of_iterations )
{
    long long start = getCurrentTimeMCS();
 
    volatile sss<T> s;
    for ( T i = 0; i < number_of_iterations; ++i )
    {
        s.t1_ = s.t2_;
        s.t2_ = s.t1_;
        s.t1_ = s.t2_;
        s.t2_ = s.t1_;
        s.t1_ = s.t2_;
        s.t2_ = s.t1_;
        s.t1_ = s.t2_;
        s.t2_ = s.t1_;
    }
 
    return getCurrentTimeMCS() - start;
}
 
long long do_loop_time( int number_of_iterations )
{
    long long start = getCurrentTimeMCS();
 
    for ( int i = 0; i < number_of_iterations; ++i )
    {
        getCurrentTimeMCS();
    }
 
    return getCurrentTimeMCS() - start;
}
 
int main( int argc, char* argv[] )
{
    if ( 1 == argc )
    {
        std::cout << "usage: perf_32_64 [number_of_iterations]" << std::endl;
        return 127;
    }
 
    int const number_of_iterations = ::atol( argv[1] );
 
    long time1 = do_loop<int>( number_of_iterations );
    long time2 = do_loop<size_t>( number_of_iterations );
    long time3 = do_loop<double>( number_of_iterations );
    long time4 = do_loop_time( number_of_iterations );
 
    std::cout << "number of iterations: " << number_of_iterations << std::endl
              << "do_loop<int>:         " << time1 << "mcs " << sizeof( sss<int>) << std::endl
              << "do_loop<size_t>:      " << time2 << "mcs " << sizeof( sss<size_t>) << std::endl
              << "do_loop<double>:      " << time3 << "mcs " << sizeof( sss<double>) << std::endl
              << "do_loop get_time:     " << time4 << "mcs " << std::endl
        ;
 
}


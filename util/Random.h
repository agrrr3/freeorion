#ifndef _Random_h_
#define _Random_h_

#if defined(_MSC_VER) && defined(int64_t)
#undef int64_t
#endif

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <ctime>

#include "Export.h"
#include "Logger.h"

/** \file
 *
 * A collection of robust and portable random number generation functors and
 * functions.
 *
 * If you need to generate one or two random numbers, prefer the functions
 * (e.g. RandomInt()).
 *
 * If you need to generate a large volume of random numbers with the same
 * parameterization, generate a functor (e.g. with a call to IntDist()) and
 * then call the functor repeatedly to generate the numbers.  This eliminates
 * the overhead associated with repeatedly contructing distributions, when you
 * call the Random*() functions.
 */

typedef boost::mt19937                                                          GeneratorType;
typedef boost::variate_generator<GeneratorType&, boost::uniform_smallint<> >    SmallIntDistType;
typedef boost::variate_generator<GeneratorType&, boost::uniform_int<> >         IntDistType;
typedef boost::variate_generator<GeneratorType&, boost::uniform_real<> >        DoubleDistType;
typedef boost::variate_generator<GeneratorType&, boost::normal_distribution<> > GaussianDistType;

/** seeds the underlying random number generator used to drive all random number distributions */
FO_COMMON_API void Seed(unsigned int seed);

/** seeds the underlying random number generator used to drive all random number distributions with 
    the current clock time */
FO_COMMON_API void ClockSeed();

/** returns a functor that provides a uniform distribution of small
    integers in the range [\a min, \a max]; if the integers desired
    are larger than 10000, use IntDist() instead */
FO_COMMON_API SmallIntDistType SmallIntDist(int min, int max);

/** returns a functor that provides a uniform distribution of integers in the range [\a min, \a max]; 
    if the integers desired are smaller than 10000, SmallIntDist() may be used instead */
IntDistType IntDist(int min, int max);

/** returns a functor that provides a uniform distribution of doubles in the range [\a min, \a max) */
FO_COMMON_API DoubleDistType DoubleDist(double min, double max);

/** returns a functor that provides a Gaussian (normal) distribution of doubles centered around \a mean, 
    with standard deviation \a sigma */
FO_COMMON_API GaussianDistType GaussianDist(double mean, double sigma);

/** returns an int from a uniform distribution of small integers in the range [\a min, \a max]; 
    if the integers desired are larger than 10000, use RandInt() instead */
FO_COMMON_API int RandSmallInt(int min, int max);

/** returns an int from a uniform distribution of integers in the range [\a min, \a max]; 
    if the integers desired are smaller than 10000, RandSmallInt() may be used instead */
FO_COMMON_API int RandInt(int min, int max);

/** returns a double from a uniform distribution of doubles in the range [0.0, 1.0) */
FO_COMMON_API double RandZeroToOne();

/** returns a double from a uniform distribution of doubles in the range [\a min, \a max) */
FO_COMMON_API double RandDouble(double min, double max);

/** returns a double from a Gaussian (normal) distribution of doubles centered around \a mean, 
    with standard deviation \a sigma */
FO_COMMON_API double RandGaussian(double mean, double sigma);

/** returns number in range 0 to one less than the integer representation of @p enum_vals_count, determined by @p seed */
template <typename T1>
FO_COMMON_API int GetIdxForSeed(const T1& enum_vals_count, const std::string& seed) {
    DebugLogger() << "hashing seed: " << seed;
    // use probably-bad but adequate for this purpose hash function to
    // convert seed into a hash value
    int hash_value = 223;
    for (size_t i = 0; i < seed.length(); ++i) {
        hash_value += (seed[i] * 61);
        hash_value %= 191;
    }
    DebugLogger() << "final hash value: " << hash_value
                  << " and returning: " << hash_value % static_cast<int>(enum_vals_count)
                  << " from 0 to " << static_cast<int>(enum_vals_count) - 1;
    return hash_value % static_cast<int>(enum_vals_count);
}


#endif // _Random_h_

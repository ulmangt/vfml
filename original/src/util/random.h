#ifndef RANDOMH
#define RANDOMH

/** \ingroup UtilAPI
*/

/** \file

\brief Generates random numbers in a number of ways, and has support
for saving and restoring the state of the random number generator.

*/


/** \brief Should be called before calling other random functions. */
void RandomInit(void);

/** \brief Returns a number between the min and max (inclusive). */
int RandomRange(int min, int max);

/** \brief Returns a random long integer. */
long RandomLong(void);

/** \brief Returns a random double from 0 - 1. */
double RandomDouble(void);

/** \brief Samples from a Gaussian with the specified parameters. */
double RandomGaussian(double mean, double stdev);

/** \brief Samples from the standard normal distribution. */
double RandomStandardNormal(void);

/** \brief Seeds the random number generator. */
void RandomSeed(unsigned int seed);

/** \brief Creates a new random state to use with RandomSetState. 

Each state represents a separate stream of repeatable random numbers.
This is implemented with initstate and setstate on Unix and is
currently not supported on windows.
*/
void *RandomNewState(unsigned int seed);

/** \brief Sets the current random state.

The state parameter should have been made by RandomNewState. This
allows you to have multiple repeatable random number sequences at
once.  Returns the previous state.
*/
void *RandomSetState(void *state);

/** \brief Use this to free any state made with RandomNewState */
void RandomFreeState(void *state);

#endif

#ifndef STATSH
#define STATSH

/** \ingroup UtilAPI
*/

/** \file

\brief Some statistical functions.

The stat module has some basic statistical functions and the
StatTracker type, which watches a series of samples, keeps some simple
summary statistics (sum and sumSquare in the current implementation),
and can report on the mean, variance, stdev, etc. of the sample.

\wish The StatTracker would track more interesting things.
*/

/** \brief Holds simple summary statistics of a sample 

See stats.h for more detail.
*/
typedef struct _StatTracker_ {
   long n;
   double sum;
   double sumSquare;
} StatTrackerStruct, *StatTracker;

/** \brief Creates a new stat tracker that is ready to have samples added to it. */
StatTracker StatTrackerNew(void);

/** \brief Frees the memory associated with the StatTracker. */
void StatTrackerFree(StatTracker st);

/** \brief Records the sample in the tracker. */
void StatTrackerAddSample(StatTracker st, double x);

/** \brief Returns the mean of the samples that have been shown to the
tracker. 

If you have not shown the tracker any samples this will probably
crash.
*/
double StatTrackerGetMean(StatTracker st);

/** \brief Returns the variance of the samples that have been shown to the tracker. 

If you have not shown the tracker at least two samples this will probably
crash.
*/
double StatTrackerGetVariance(StatTracker st);

/** \brief Returns the standard deviation of the samples that have been shown to the tracker.

If you have not shown the tracker at least two samples this will
probably crash.
*/
double StatTrackerGetStdev(StatTracker st);

/** \brief Returns the number of samples shown to the stat tracker. */
long StatTrackerGetNumSamples(StatTracker st);

/** \brief Assume Gaussian and return a high confidence bound of the sample .

Assumes a Gaussian distribution and returns the distance from the mean
within which 1 - delta % of the mass lies. This calls the GetStdev
function internally and will crash if that crashes.
*/
double StatTrackerGetNormalBound(StatTracker st, double delta);

/** \brief Return a high confidence bound from the normal distribution.

Assumes a Gaussian distribution and returns the distance from the mean
within which 1 - delta % of the mass lies (esentially the same
function as StatTrackerGetNormalBound).

*/
double StatGetNormalBound(double variance, long n, double delta);


/** \brief  Returns the one sided Hoeffding bound.

With n observations of a variable with the specified range. That is,
the true value is less than the observed mean + this function's result
with probability 1 - delta.
*/
double StatHoeffdingBoundOne(double range, double delta, long n);

/** \brief Returns the two sided Hoeffding bound.

With n observations of a variable with the specified range. That is,
the true value is within this function's result of the obseved mean
with probability 1 - delta.
*/
double StatHoeffdingBoundTwo(double range, double delta, long n);

/** \brief Returns the log of the gamma function of xx. 

This function is lifted from Numerical Recipes in C. 
*/
float StatLogGamma(float xx);

#endif /* STATSH */

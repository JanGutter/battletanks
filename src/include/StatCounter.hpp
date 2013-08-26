#ifndef __STATCOUNTER_HPP__
#define __STATCOUNTER_HPP__

#include <stdlib.h>
/*
 * Stores stats without keep track of the actual numbers.
 *
 * Implements Knuth's online algorithm for variance, first one
 * found under "Online Algorithm" of
 * http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
 *
 * Uses the incremental algorithm for updating the mean found at
 * http://webdocs.cs.ualberta.ca/~sutton/book/2/node6.html
 * P.O.D structure
 */

class StatCounter {
public:
#if STAT_SUM
	double running_sum;
#endif
	double m2;
	double running_mean;
	unsigned long int n;
	// reinitialise
	void init();
	// add a sample
	void push(double sample);
	// add the mean as a sample
	// (sum() will be invalid)
	void pushmean();
	// sample variance
	double variance() const;
	// sample mean
	double mean();
	// sum of samples
#if STAT_SUM
	double sum() const;
#endif
	// number of samples
	unsigned long int count();

};

/* Initialise */
inline void StatCounter::init() {
	n = 0;
#if STAT_SUM
	running_sum = 0.0;
#endif
	m2 = 0.0;
	running_mean = 0.0;
}

/* add a sample */
inline void StatCounter::push(double sample) {
#if STAT_SUM
	running_sum += sample;
#endif
	n++;
	double delta = sample - running_mean;
	running_mean += delta / n;
	m2 += delta*(sample - running_mean);
}

/* don't touch the mean, just increase the count */
/* after using this, stats'll make no sense, variance() should be decreasing */
/* running_sum() will be invalid */

inline void StatCounter::pushmean() {
	n++;
}

/* sample variance */
inline double StatCounter::variance() const {
	return m2 / n;
}

/* sample mean */
inline double StatCounter::mean() {
	return running_mean;
}

#if STAT_SUM
/* sum of all samples */
inline double StatCounter::sum() const {
	return running_sum;
}
#endif

/* number of samples */
inline unsigned long int StatCounter::count() {
	return n;
}


// Online computation of the sample covariance between two random variables
//   See "Covariance" at:
//   http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
// P.O.D Structure
class SampleCovPair {
public:
	StatCounter x;
	StatCounter y;
	StatCounter xy;
	// reset the statistics
	void init();
	// add a new data point
	void push(double _x, double _y);
	// unbiased estimate of the covariance
	double covariance();
	// number of data points estimate is based on
	unsigned long int count();
};

/* reset the covariance statistics */
inline void SampleCovPair::init() {
	x.init();
	y.init();
	xy.init();
}

/* add a new data point */
inline void SampleCovPair::push(double _x, double _y) {
	x.push(_x);
	y.push(_y);
	xy.push(_x*_y);
}

/* unbiased estimate of the covariance */
inline double SampleCovPair::covariance() {
	return xy.mean() - x.mean() * y.mean();
}

/* how many data points is our covariance estimate based on? */
inline unsigned long int SampleCovPair::count() {
	return x.count();
}

#endif  // __STATISTICS_HPP__


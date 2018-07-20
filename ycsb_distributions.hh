#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <mutex>
#include <random>

namespace ycsbc {

double rand_val(int seed){
	const long  a =      16807;  // Multiplier
	const long  m = 2147483647;  // Modulus
	const long  q =     127773;  // m div a
	const long  r =       2836;  // m mod a
	static long x;               // Random int value
	long        x_div_q;         // x divided by q
	long        x_mod_q;         // x modulo q
	long        x_new;           // New x value

	// Set the seed if argument is non-zero and then return zero
	if (seed > 0){
		x = seed;
		return(0.0);
	}

	// RNG using integer arithmetic
	x_div_q = x / q;
	x_mod_q = x % q;
	x_new = (a * x_mod_q) - (r * x_div_q);
	if (x_new > 0)
		x = x_new;
	else
		x = x_new + m;

	// Return a random value between 0.0 and 1.0
	return((double) x / m);
}

struct UpperBound{
	int n;

	UpperBound():n(0){}

	void init(int n_){
		n = n_;
	}
};

struct UniformDist: public kvrandom_lcg_nr, public UpperBound{
	int next(){
		return kvrandom_lcg_nr::next() % UpperBound::n;
	}
};

struct ZipfianDist: public UpperBound{
	double c = 0;          // Normalization constant
	double *sum_probs;     // Pre-calculated sum of probabilities


	ZipfianDist():sum_probs(nullptr){}

	~ZipfianDist(){
		if(sum_probs)
			free(sum_probs);
	}

	void init(int n_, double alpha_=0.99){
		UpperBound::init(n_);
		first(alpha_);
		rand_val(1);
	}

	void first(double alpha){
		assert(n);
		for (int i=1; i<=n; i++){
			c = c + (1.0 / pow((double) i, alpha));
		}
		c = 1.0 / c;

		sum_probs = (double*)malloc((n+1)*sizeof(*sum_probs));
		sum_probs[0] = 0;
		for (int i=1; i<=n; i++) {
			sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
		}
	}

	int next(){
		double z;                     // Uniform random number (0 < z < 1)
		int zipf_value;               // Computed exponential value to be returned

		// Pull a uniform random number (0 < z < 1)
		do{
			z = rand_val(0);
		}while ((z == 0) || (z == 1));

		// Map z to the value
		int low = 1, high = n, mid;
		do {
			mid = floor((low+high)/2);
			if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
				zipf_value = mid;
				break;
			}else if (sum_probs[mid] >= z) {
				high = mid-1;
			}else {
				low = mid+1;
			}
		} while (low <= high);

		// Assert that zipf_value is between 1 and N
		assert((zipf_value >=1) && (zipf_value <= n));

		return(zipf_value);
	}
};


}; //ycsb

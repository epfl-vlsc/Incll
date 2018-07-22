#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <mutex>
#include <random>

namespace ycsbc {

double rand_val(){
	static kvrandom_lcg_nr rand;
	const static long  m = 2147483647;  // Modulus

	// Return a random value between 0.0 and 1.0
	return((double) rand.next() / m);
}

const long FNV_OFFSET_BASIS_64 = 0xCBF29CE484222325;
const long FNV_PRIME_64 = 1099511628211;
inline long fnvhash64(long val) {
	long hash = FNV_OFFSET_BASIS_64;

	for (int i = 0; i < 8; i++) {
		long octet = val & 0x00ff;
		val = val >> 8;

		hash = hash ^ octet;
		hash = hash * FNV_PRIME_64;
	}
	return abs(hash);
}

struct UpperBound{
	long n;

	UpperBound():n(0){}

	void init(long n_){
		n = n_;
	}

	void init(long n_, void* ptr){
		n = n_;
		(void)(ptr);
	}
};

struct CounterGen: public UpperBound{
	std::atomic<long> counter;
	long n_low;

	CounterGen():UpperBound(),
			counter(0), n_low(0){}

	long next(){
		assert(n);
		return counter.fetch_add(1) % n;
	}

	long last(){
		return (counter.load() - 1) % n;
	}

	void init(long n_, long n_low_ = 0){
		n = n_;
		n_low = n_low_;
	}
};

struct UniformGen: public kvrandom_lcg_nr, public UpperBound{
	long next(){
		return kvrandom_lcg_nr::next() % n;
	}
};

struct ZipfianGen: public UpperBound{
	double c = 0;          // Normalization constant
	double *sum_probs;     // Pre-calculated sum of probabilities

	ZipfianGen():sum_probs(nullptr){}

	~ZipfianGen(){
		if(sum_probs)
			free(sum_probs);
	}

	void init(int n_, double alpha_=0.99){
		UpperBound::init(n_);
		first(alpha_);
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

	long next(){
		return next(n);
	}

	long next(long n_){
		double z;                     // Uniform random number (0 < z < 1)
		long zipf_value = 0;           // Computed exponential value to be returned

		// Pull a uniform random number (0 < z < 1)
		do{
			z = rand_val();
		}while ((z == 0) || (z == 1));

		// Map z to the value
		int low = 0, high = n_, mid;
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
		assert((zipf_value >=1) && (zipf_value <= n_));

		return zipf_value-1;
	}
};

struct ScrambledZipfianGen: public ZipfianGen{
	long next(){
		return fnvhash64(ZipfianGen::next()) % n;
	}
};

struct SkewedLatestGen: public ZipfianGen{
	CounterGen *insert_rand;

	void init(int n_, CounterGen *insert_rand_){
		ZipfianGen::init(n_);
		insert_rand = insert_rand_;
	}

	long next(){
		long max = insert_rand->last();
		return max - ZipfianGen::next(max);
	}
};

}; //ycsb

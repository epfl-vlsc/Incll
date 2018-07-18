//
//
//  Created by Jinglei Ren on 12/7/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <mutex>
#include <random>

namespace ycsbc {

inline double RandomDouble(double min = 0.0, double max = 1.0) {
  static std::default_random_engine generator;
  static std::uniform_real_distribution<double> uniform(min, max);
  return uniform(generator);
}

class Generator {
public:
	virtual uint64_t next() = 0;
	virtual uint64_t last() = 0;
	virtual ~Generator() {}
};
class UniformGenerator : public Generator{
public:
	// Both min and max are inclusive
	UniformGenerator(uint64_t min, uint64_t max):
		dist_(min, max) {
		next();
	}

	UniformGenerator(uint64_t num_items):
		dist_(0, num_items - 1) {
	}


	uint64_t next();
	uint64_t last();

private:
	std::mt19937_64 generator_;
	std::uniform_int_distribution<uint64_t> dist_;
	uint64_t last_int_;
	std::mutex mutex_;
};

inline uint64_t UniformGenerator::next() {
	std::lock_guard<std::mutex> lock(mutex_);
	return last_int_ = dist_(generator_);
}

inline uint64_t UniformGenerator::last() {
	std::lock_guard<std::mutex> lock(mutex_);
	return last_int_;
}

class ZipfianGenerator : public Generator {
public:
	constexpr static const double kZipfianConst = 0.99;
	static const uint64_t kMaxNumItems = (UINT64_MAX >> 24);

	ZipfianGenerator(uint64_t min, uint64_t max,
			double zipfian_const = kZipfianConst) :
		num_items_(max - min + 1), base_(min), theta_(zipfian_const),
		zeta_n_(0), n_for_zeta_(0) {
		assert(num_items_ >= 2 && num_items_ < kMaxNumItems);
		zeta_2_ = Zeta(2, theta_);
		alpha_ = 1.0 / (1.0 - theta_);
		RaiseZeta(num_items_);
		eta_ = Eta();

		next();
	}

	ZipfianGenerator(uint64_t num_items) :
		ZipfianGenerator(0, num_items - 1, kZipfianConst) {}

	uint64_t next(uint64_t num_items);

	uint64_t next() {
		return next(num_items_);
	}

	uint64_t last();

private:
	///
	/// Compute the zeta constant needed for the distribution.
	/// Remember the number of items, so if it is changed, we can recompute zeta.
	///
	void RaiseZeta(uint64_t num) {
		assert(num >= n_for_zeta_);
		zeta_n_ = Zeta(n_for_zeta_, num, theta_, zeta_n_);
		n_for_zeta_ = num;
	}

	double Eta() {
		return (1 - std::pow(2.0 / num_items_, 1 - theta_))
			/ (1 - zeta_2_ / zeta_n_);
	}

	///
	/// Calculate the zeta constant needed for a distribution.
	/// Do this incrementally from the last_num of items to the cur_num.
	/// Use the zipfian constant as theta. Remember the new number of items
	/// so that, if it is changed, we can recompute zeta.
	///
	static double Zeta(uint64_t last_num, uint64_t cur_num,
			 double theta, double last_zeta) {
		double zeta = last_zeta;
		for (uint64_t i = last_num + 1; i <= cur_num; ++i) {
			zeta += 1 / std::pow(i, theta);
		}
		return zeta;
	}

	static double Zeta(uint64_t num, double theta) {
		return Zeta(0, num, theta, 0);
	}

	uint64_t num_items_;
	uint64_t base_; /// Min number of items to generate

	// Computed parameters for generating the distribution
	double theta_, zeta_n_, eta_, alpha_, zeta_2_;
	uint64_t n_for_zeta_; /// Number of items used to compute zeta_n
	uint64_t last_value_;
	std::mutex mutex_;
};

inline uint64_t ZipfianGenerator::next(uint64_t num) {
	assert(num >= 2 && num < kMaxNumItems);
	std::lock_guard<std::mutex> lock(mutex_);

	if (num > n_for_zeta_) { // Recompute zeta_n and eta
		RaiseZeta(num);
		eta_ = Eta();
	}

	double u = RandomDouble();
	double uz = u * zeta_n_;

	if (uz < 1.0) {
		return last_value_ = 0;
	}

	if (uz < 1.0 + std::pow(0.5, theta_)) {
		return last_value_ = 1;
	}

	return last_value_ = base_ + num * std::pow(eta_ * u - eta_ + 1, alpha_);
}

inline uint64_t ZipfianGenerator::last() {
	std::lock_guard<std::mutex> lock(mutex_);
	return last_value_;
}

}; //ycsb

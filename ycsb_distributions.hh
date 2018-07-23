/*
 * zipf.h
 *
 *  Created on: 13 Jun 2018
 *      Author: ncohen
 */


#include <algorithm>
#include <cmath>
#include <random>

/** Zipf-like random distribution.
 *
 * "Rejection-inversion to generate variates from monotone discrete
 * distributions", Wolfgang Hörmann and Gerhard Derflinger
 * ACM TOMACS 6.3 (1996): 169-184
 */
template<class IntType = unsigned long, class RealType = double>
class ZipfianDist{
public:
    typedef RealType input_type;
    typedef IntType result_type;

    std::mt19937 rng;

    static_assert(std::numeric_limits<IntType>::is_integer, "");
    static_assert(!std::numeric_limits<RealType>::is_integer, "");

    ZipfianDist(const IntType n=std::numeric_limits<IntType>::max(),
                      const RealType q=0.99): n(n), q(q), H_x1(H(1.5) - 1.0)
    , H_n(H(n + 0.5)), dist(H_x1, H_n){}

    void reset(int seed){
    	rng.seed(seed);
    }

    IntType next(){
        while (true) {
            const RealType u = dist(rng);
            const RealType x = H_inv(u);
            const IntType  k = clamp<IntType>(std::round(x), 1, n);
            if (u >= H(k + 0.5) - h(k)) {
                return k;
            }
        }
    }

protected:
    /** Clamp x to [min, max]. */
    template<typename T>
    static constexpr T clamp(const T x, const T min, const T max)
    {
        return std::max(min, std::min(max, x));
    }

    /** exp(x) - 1 / x */
    static double
    expxm1bx(const double x)
    {
        return (std::abs(x) > epsilon)
        ? std::expm1(x) / x
        : (1.0 + x/2.0 * (1.0 + x/3.0 * (1.0 + x/4.0)));
    }

    /** H(x) = log(x) if q == 1, (x^(1-q) - 1)/(1 - q) otherwise.
     * H(x) is an integral of h(x).
     *
     * Note the numerator is one less than in the paper order to work with all
     * positive q.
     */
    const RealType H(const RealType x)
    {
        const RealType log_x = std::log(x);
        return expxm1bx((1.0 - q) * log_x) * log_x;
    }

    /** log(1 + x) / x */
    static RealType
    log1pxbx(const RealType x)
    {
        return (std::abs(x) > epsilon)
        ? std::log1p(x) / x
        : 1.0 - x * ((1/2.0) - x * ((1/3.0) - x * (1/4.0)));
    }

    /** The inverse function of H(x) */
    const RealType H_inv(const RealType x)
    {
        const RealType t = std::max(-1.0, x * (1.0 - q));
        return std::exp(log1pxbx(t) * x);
    }

    /** That hat function h(x) = 1 / (x ^ q) */
    const RealType h(const RealType x)
    {
        return std::exp(-q * std::log(x));
    }

    static constexpr RealType epsilon = 1e-8;

    IntType                                  n;     ///< Number of elements
    RealType                                 q;     ///< Exponent
    RealType                                 H_x1;  ///< H(x_1)
    RealType                                 H_n;   ///< H(n)
    std::uniform_real_distribution<RealType> dist;  ///< [H(x_1), H(n)]
};


const uint64_t kFNVOffsetBasis64 = 0xCBF29CE484222325;
const uint64_t kFNVPrime64 = 1099511628211;
template <class IntType>
inline uint64_t FNVHash64(IntType val) {
	uint64_t hash = kFNVOffsetBasis64;

  for (int i = 0; i < 8; i++) {
	  uint64_t octet = val & 0x00ff;
    val = val >> 8;

    hash = hash ^ octet;
    hash = hash * kFNVPrime64;
  }
  return hash;
}

template<class IntType = unsigned long, class RealType = double>
class ScrambledZipfianDist: public ZipfianDist<IntType, RealType>{
public:
	ScrambledZipfianDist(
			const IntType n=std::numeric_limits<IntType>::max(), double insert_ratio=0.5,
			int op_count=1000000, const RealType q=0.99):
				ZipfianDist<IntType, RealType>(n + 2 * op_count * insert_ratio, q){}

	IntType next(){
		return scramble(ZipfianDist<IntType, RealType>::next());
	}

	IntType scramble(IntType rand_num){
		return FNVHash64(rand_num) % ZipfianDist<IntType, RealType>::n;
	}
};

typedef kvrandom_lcg_nr UniGen;
typedef ZipfianDist<uint32_t, double> ZipGen;
typedef ScrambledZipfianDist<uint32_t, double> ScrambledZipGen;


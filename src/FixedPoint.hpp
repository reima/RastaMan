#ifndef FIXEDPOINT_HPP
#define FIXEDPOINT_HPP

#include <limits>
#include <boost/static_assert.hpp>
#include <boost/operators.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/integer.hpp>
#include <boost/mpl/if.hpp>

#include <cstdint>

template<
  typename BaseType,
  std::size_t FracBits,
  std::size_t IntBits = std::numeric_limits<BaseType>::digits - FracBits
>
class FixedPoint
    : boost::ordered_field_operators<FixedPoint<BaseType, FracBits, IntBits>
    , boost::unit_steppable<FixedPoint<BaseType, FracBits, IntBits> > > {
 private:
  BOOST_STATIC_ASSERT(std::numeric_limits<BaseType>::is_integer);
  BOOST_STATIC_ASSERT(FracBits + IntBits == std::numeric_limits<BaseType>::digits);

  typedef typename boost::mpl::if_c<
    std::numeric_limits<BaseType>::is_signed,
    typename boost::int_t<std::numeric_limits<BaseType>::digits * 2>::least,
    typename boost::uint_t<std::numeric_limits<BaseType>::digits * 2>::least
  >::type PromoteType;

  static const BaseType kOne = (1 << FracBits);

 public:
  typedef FixedPoint<BaseType, FracBits, IntBits> type;
  typedef BaseType base_type;
  static const std::size_t frac_bits = FracBits;
  static const std::size_t int_bits = IntBits;

  FixedPoint() {
  }

  template<typename T>
  FixedPoint(T value) {
    Set<T>(value);
  }

 public:
  template<typename Float>
  typename boost::enable_if<boost::is_floating_point<Float> >::type
  Set(Float f) {
    value_ = static_cast<BaseType>(f * kOne + (f >= 0 ? .5 : -.5));
  }

  template<typename Int>
  typename boost::enable_if<boost::is_integral<Int> >::type
  Set(Int i) {
    value_ = i * kOne;
  }

  template<typename Float>
  typename boost::enable_if<boost::is_floating_point<Float>, Float>::type
  GetAs() {
    return static_cast<Float>(value_) / kOne;
  }

  template<typename Int>
  typename boost::enable_if<boost::is_integral<Int>, Int>::type
  GetAs() {
    return value_ / kOne;
  }

  bool operator<(const type& other) const {
    return value_ < other.value_;
  }

  bool operator==(const type& other) const {
    return value_ == other.value_;
  }

  type& operator+=(const type& other) {
    value_ += other.value_;
    return *this;
  }

  type& operator-=(const type& other) {
    value_ -= other.value_;
    return *this;
  }

  typename boost::enable_if_c<std::numeric_limits<BaseType>::is_signed, type>::type
  operator-() const {
    return type(-value_, 0);
  }

  type& operator*=(const type& other) {
    value_ = (static_cast<PromoteType>(value_) * other.value_) >> FracBits;
    return *this;
  }

  type& operator/=(const type& other) {
    value_ = (static_cast<PromoteType>(value_) << FracBits) / other.value_;
    return *this;
  }

  type& operator++() {
    value_ += kOne;
    return *this;
  }

  type& operator--() {
    value_ -= kOne;
    return *this;
  }

  //operator bool() const {
  //  return value_ != 0;
  //}

  //bool operator!() const {
  //  return value_ == 0;
  //}

 private:
  FixedPoint(BaseType value, int) : value_(value) {
  }

  friend class std::numeric_limits<type>;

  BaseType value_;
};

namespace std {
template<typename B, std::size_t F, std::size_t I>
class numeric_limits<FixedPoint<B, F, I> > {
 public:
  typedef FixedPoint<B, F, I> T;
  static const bool is_specialized = true;
  static T min() throw() {
    return T(numeric_limits<B>::min(), 0);
  }
  static T max() throw() {
    return T(numeric_limits<B>::max(), 0);
  }
  static const int digits = I;
  static const int digits10 = static_cast<int>(I * /* log10(2) */ .30103 + .5);
  static const bool is_signed = numeric_limits<B>::is_signed;
  static const bool is_integer = false;
  static const bool is_exact = true;
  static const bool radix = 0;
  static T epsilon() throw() {
    return T(1, 0);
  }
  static T round_error() throw() {
    return epsilon();
  }

  static const int min_exponent = 0;
  static const int min_exponent10 = 0;
  static const int max_exponent = 0;
  static const int max_exponent10 = 0;

  static const bool has_infinity = false;
  static const bool has_quiet_NaN = false;
  static const bool has_signaling_NaN = false;
  static const float_denorm_style has_denorm = denorm_absent;
  static const bool has_denorm_loss = false;
  static T infinity() throw() { return T(0); }
  static T quiet_NaN() throw() { return T(0); }
  static T signaling_NaN() throw() { return T(0); }
  static T denorm_min() throw() { return T(0); }

  static const bool is_iec559 = false;
  static const bool is_bounded = true;
  static const bool is_modulo = true;

  static const bool traps = false;
  static const bool tinyness_before = false;
  static const float_round_style round_style = round_toward_zero;

};
} // namespace std

#endif
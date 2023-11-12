#include "big_integer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <vector>
static constexpr size_t EXP = 32;
static constexpr uint64_t BASE = (1LL << EXP);
static constexpr uint32_t INT_MOD = 1000000000;
static constexpr uint32_t INT_BASE = 10;
static constexpr size_t DIGITS_CNT = 9;

uint32_t exp(uint32_t base, size_t power) {
  uint32_t result = 1;
  for (; power > 0; --power) {
    result *= base;
  }
  return result;
}

template <typename T>
static uint32_t ui_cast(T value) {
  return static_cast<uint32_t>(value & std::numeric_limits<uint32_t>::max());
}

template <typename T>
static uint64_t ull_cast(T value) {
  return static_cast<uint64_t>(value);
}

size_t big_integer::length() const {
  return _data.size();
}

uint32_t& big_integer::operator[](size_t index) {
  return _data[index];
}

uint32_t big_integer::operator[](size_t index) const {
  return _data[index];
}

void big_integer::trim() {
  while (length() > 0 && _data.back() == 0) {
    _data.pop_back();
  }
}

bool big_integer::eq_zero() const {
  return length() == 0;
}

bool big_integer::abs_great_or_eq(const big_integer& rhs) const {
  if (length() != rhs.length()) {
    return length() > rhs.length();
  }
  for (size_t i = length(); i > 0; i--) {
    if (_data[i - 1] != rhs[i - 1]) {
      return _data[i - 1] > rhs[i - 1];
    }
  }
  return true;
}

big_integer mul_uint(const big_integer& a, const uint32_t& b) {
  big_integer result;
  uint64_t carry = 0;
  for (size_t i = 0; i < a.length(); i++) {
    uint64_t tmp = static_cast<uint64_t>(a[i]) * b + carry;
    result._data.push_back(ui_cast(tmp));
    carry = tmp >> EXP;
  }
  if (carry != 0) {
    result._data.push_back(ui_cast(carry));
  }
  return result;
}

void swap(big_integer& a, big_integer& b) {
  a._data.swap(b._data);
  std::swap(a._sign, b._sign);
}

big_integer::big_integer() = default;

big_integer::big_integer(int a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(long long a) {
  if (a != 0) {
    uint64_t b = ull_cast(a);
    _sign = a < 0;
    if (_sign) {
      b = -b;
    }
    _data = {ui_cast(b)};
    if (b >> EXP) {
      _data.push_back(ui_cast(b >> EXP));
    }
  }
}

big_integer::big_integer(unsigned long long a) {
  _data = {ui_cast(a), ui_cast(a >> EXP)};
  trim();
}

big_integer::big_integer(const std::string& str) {
  big_integer result;
  size_t i = 0;
  bool sign = false;
  if (str[0] == '-') {
    sign = true;
    i++;
  }
  if (i == str.size()) {
    throw std::invalid_argument("String must be not empty");
  }
  for (; i < str.size(); i += DIGITS_CNT) {
    uint32_t tmp = 0;
    for (uint32_t j = 0; j < std::min(DIGITS_CNT, str.size() - i); j++) {
      char ch = str[i + j];
      if (ch < '0' || '9' < ch) {
        throw std::invalid_argument("String characters must be ints");
      }
      tmp *= INT_BASE;
      tmp += ch - '0';
    }
    if (str.size() - i >= DIGITS_CNT) {
      result = mul_uint(result, INT_MOD);
    } else {
      result = mul_uint(result, exp(INT_BASE, str.size() - i));
    }
    result += tmp;
  }
  result._sign = sign;
  swap(*this, result);
  trim();
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(const big_integer& other) = default;

static std::pair<uint32_t, uint32_t> add(uint32_t a, uint32_t b) {
  return std::pair(a + b, UINT32_MAX - b < a);
}

static std::pair<uint32_t, uint32_t> sub(const uint32_t& a, const uint32_t& b) {
  return std::pair(a - b, a < b);
}

void big_integer::bit_negation(big_integer& a) {
  for (auto& i : a._data) {
    i = ~i;
  }
  if (a._sign) {
    --a;
  } else {
    ++a;
  }
}

big_integer& big_integer::shift_left(big_integer& a, size_t shift) {
  a._data.insert(a._data.begin(), shift, 0);
  return a;
}

bool big_integer::sub_in_pos(big_integer& lhs, const big_integer& rhs, size_t pos) {
  uint32_t carry = 0;
  for (size_t i = 0; i < rhs.length() || carry > 0; i++) {
    auto p = sub(lhs[i + pos], carry);
    carry = p.second;
    if (i < rhs.length()) {
      p = sub(p.first, rhs[i]);
    }
    carry = std::max(p.second, carry);
    lhs[i + pos] = p.first;
  }
  return carry;
}

template <bool ignore_last_carry>
void big_integer::add_in_pos(big_integer& lhs, const big_integer& rhs, size_t pos) {
  uint32_t carry = 0;
  for (size_t i = 0; i < rhs.length() || (carry && !ignore_last_carry); i++) {
    if (i + pos == lhs.length()) {
      lhs._data.push_back(0);
    }
    auto p = add(lhs[i + pos], carry);
    carry = p.second;
    if (i < rhs.length()) {
      p = add(p.first, rhs[i]);
    }
    carry = std::max(p.second, carry);
    lhs[i + pos] = p.first;
  }
}

template <bool ignore_sign>
void big_integer::add_with_ignore(const big_integer& rhs) {
  if (!ignore_sign && _sign != rhs._sign) {
    sub_with_ignore<true>(rhs);
  } else {
    add_in_pos<false>(*this, rhs, 0);
    trim();
  }
}

template <bool ignore_sign>
void big_integer::sub_with_ignore(const big_integer& rhs) {
  if (!ignore_sign && _sign != rhs._sign) {
    add_with_ignore<true>(rhs);
  } else {
    if (abs_great_or_eq(rhs)) {
      sub_in_pos(*this, rhs, 0);
    } else {
      big_integer tmp = rhs;
      tmp.sub_with_ignore<ignore_sign>(*this);
      tmp._sign ^= !ignore_sign;
      swap(*this, tmp);
    }
    trim();
  }
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  add_with_ignore<false>(rhs);
  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  sub_with_ignore<false>(rhs);
  return *this;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  big_integer result;
  result._data.resize(length() + rhs.length());
  for (size_t i = 0; i < length(); i++) {
    uint64_t carry = 0;
    for (size_t j = 0; j < rhs.length() || carry > 0; j++) {
      uint64_t cur = result[i + j] + ull_cast(_data[i]) * (j < rhs.length() ? rhs[j] : 0) + carry;
      result[i + j] = ui_cast(cur);
      carry = cur >> EXP;
    }
  }
  result._sign = _sign ^ rhs._sign;
  result.trim();
  swap(*this, result);
  return *this;
}

template <bool return_remainder>
big_integer& big_integer::abstract_division(const big_integer& rhs) {
  if (rhs == 0) {
    throw std::invalid_argument("Cannot divide by zero");
  }
  if (rhs.length() == 1) {
    if (return_remainder) {
      auto p = *this;
      p.div_uint(rhs[0]);
      p._sign ^= rhs._sign;
      *this -= p * rhs;
    } else {
      div_uint(rhs[0]);
      _sign ^= rhs._sign;
    }
    return *this;
  }
  big_integer dividend = *this;
  big_integer divisor = rhs;
  dividend._sign = false;
  divisor._sign = false;
  if (divisor > dividend) {
    return return_remainder ? *this : *this = 0;
  }
  uint64_t d = BASE / (ull_cast(rhs._data.back()) + 1);
  size_t divisor_len = rhs.length();
  size_t len_diff = length() - divisor_len;
  if (d != 1) {
    dividend = mul_uint(dividend, d);
    divisor = mul_uint(divisor, d);
  }
  if (dividend.length() == len_diff + divisor_len) {
    dividend._data.push_back(0);
  }
  std::vector<uint32_t> q;
  for (size_t j = len_diff + 1; j > 0; j--) {
    uint64_t two_digits = ((ull_cast(dividend[j + divisor_len - 1]) << EXP) + dividend[j + divisor_len - 2]);
    uint64_t poss_q = two_digits / divisor[divisor_len - 1];
    uint64_t poss_r = two_digits % divisor[divisor_len - 1];
    assert(divisor_len >= 2);
    assert(j + divisor_len >= 3);
    if (poss_q == BASE || poss_q * divisor[divisor_len - 2] > BASE * poss_r + dividend[j + divisor_len - 3]) {
      --poss_q;
      poss_r += divisor[divisor_len - 1];
      if (poss_r < BASE &&
          (poss_q == BASE || poss_q * divisor[divisor_len - 2] > BASE * poss_r + dividend[j + divisor_len - 3])) {
        --poss_q;
      }
    }
    big_integer try_subtrahend = mul_uint(divisor, poss_q);
    bool flag = sub_in_pos(dividend, try_subtrahend, j - 1);
    if (flag) {
      --poss_q;
      add_in_pos<true>(dividend, divisor, j - 1);
    }
    q.push_back(poss_q);
  }
  if (return_remainder) {
    std::swap(_data, dividend._data);
    div_uint(d);
  } else {
    std::reverse(q.begin(), q.end());
    std::swap(_data, q);
    _sign ^= rhs._sign;
  }
  trim();
  return *this;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  return *this = abstract_division<false>(rhs);
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  return *this = abstract_division<true>(rhs);
}

template <class Operation>
void big_integer::bit_operation(const big_integer& rhs) {
  Operation operation;
  size_t len = std::max(length(), rhs.length());
  _data.resize(len);
  if (_sign) {
    bit_negation(*this);
  }
  big_integer b = rhs;
  if (rhs._sign) {
    bit_negation(b);
  }
  for (size_t i = 0; i < len; i++) {
    _data[i] = operation(_data[i], (i < b.length()) ? b[i] : 0);
  }
  _sign = operation(_sign, rhs._sign);
  if (_sign) {
    bit_negation(*this);
  }
  trim();
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  bit_operation<std::bit_and<uint32_t>>(rhs);
  return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  bit_operation<std::bit_or<uint32_t>>(rhs);
  return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  bit_operation<std::bit_xor<uint32_t>>(rhs);
  return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
  size_t size = length();
  size_t big_shift = rhs / EXP;
  size_t small_shift = rhs % EXP;
  _data.resize(length() + big_shift + 1);
  for (size_t i = size; i > 0; i--) {
    _data[i + big_shift] |= _data[i - 1] >> std::min(EXP - 1, EXP - small_shift);
    _data[i + big_shift - 1] = _data[i - 1] << (small_shift);
  }
  for (size_t i = big_shift; i > 0; i--) {
    _data[i - 1] = 0;
  }
  trim();
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  size_t size = length();
  if (_sign) {
    bit_negation(*this);
  }
  size_t big_shift = rhs / EXP;
  size_t small_shift = rhs % EXP;
  uint32_t complete = _sign ? ui_cast(BASE - 1) : 0;
  for (size_t i = 0; i < size - big_shift; i++) {
    _data[i] = _data[i + big_shift] >> small_shift;
    _data[i] |= ((i + big_shift + 1 < _data.size()) ? _data[i + big_shift + 1] : complete)
             << std::min(EXP - 1, EXP - small_shift);
  }
  _data.resize(size - big_shift);
  if (_sign) {
    bit_negation(*this);
  }
  trim();
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  big_integer tmp = *this;
  tmp._sign = !tmp._sign;
  return tmp;
}

big_integer big_integer::operator~() const {
  big_integer result = *this;
  result._sign = !result._sign;
  --result;
  return result;
}

big_integer& big_integer::operator++() {
  if (!_sign) {
    uint32_t carry = 1;
    for (size_t i = 0; i < _data.size() && carry > 0; ++i) {
      uint64_t sum = _data[i] + carry;
      _data[i] = ui_cast(sum);
      carry = sum >> EXP;
    }
    if (carry > 0) {
      _data.push_back(ui_cast(carry));
    }
  } else {
    _sign = false;
    --(*this);
    if (!eq_zero()) {
      _sign = true;
    }
  }
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer tmp = *this;
  ++(*this);
  return tmp;
}

big_integer& big_integer::operator--() {
  if (!_sign) {
    if (eq_zero()) {
      _sign = true;
      _data.push_back(1);
    } else {
      uint32_t carry = 1;
      for (size_t i = 0; i < _data.size() && carry > 0; ++i) {
        uint64_t sum = _data[i] - carry;
        _data[i] = ui_cast(sum);
        carry = sum >> EXP;
      }
    }
    trim();
  } else {
    _sign = false;
    ++(*this);
    _sign = true;
  }
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer tmp = big_integer(*this);
  --(*this);
  return tmp;
}

big_integer operator+(const big_integer& a, const big_integer& b) {
  return big_integer(a) += b;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
  return big_integer(a) -= b;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
  return big_integer(a) *= b;
}

big_integer operator/(const big_integer& a, const big_integer& b) {
  return big_integer(a) /= b;
}

big_integer operator%(const big_integer& a, const big_integer& b) {
  return big_integer(a) %= b;
}

big_integer operator&(const big_integer& a, const big_integer& b) {
  return big_integer(a) &= b;
}

big_integer operator|(const big_integer& a, const big_integer& b) {
  return big_integer(a) |= b;
}

big_integer operator^(const big_integer& a, const big_integer& b) {
  return big_integer(a) ^= b;
}

big_integer operator<<(const big_integer& a, int b) {
  return big_integer(a) <<= b;
}

big_integer operator>>(const big_integer& a, int b) {
  return big_integer(a) >>= b;
}

bool operator==(const big_integer& a, const big_integer& b) {
  return (a.eq_zero() && b.eq_zero()) || (a._sign == b._sign && a._data == b._data);
}

bool operator>(const big_integer& a, const big_integer& b) {
  if (a._sign != b._sign) {
    return !a._sign;
  }
  if (a._data.size() != b._data.size()) {
    return (a._data.size() > b._data.size()) ^ a._sign;
  }

  for (size_t i = a.length(); i > 0; --i) {
    if (a[i - 1] != b[i - 1]) {
      return (a[i - 1] > b[i - 1]) ^ a._sign;
    }
  }

  return false;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(b > a);
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  return !(a >= b);
}

std::string to_string(const big_integer& a) {
  std::string s;
  big_integer tmp_integer = a;
  while (tmp_integer != 0 || s.empty()) {
    big_integer reminder(tmp_integer % INT_MOD);
    uint32_t reminder_uint = (reminder.length()) ? reminder[0] : 0;
    tmp_integer /= INT_MOD;
    std::string str_to_add = std::to_string(reminder_uint);
    std::reverse(str_to_add.begin(), str_to_add.end());
    if (tmp_integer != 0) {
      while (str_to_add.size() < DIGITS_CNT) {
        str_to_add += '0';
      }
    }
    s += str_to_add;
  }

  if (a._sign && !a.eq_zero()) {
    s.push_back('-');
  }
  std::reverse(s.begin(), s.end());
  return s;
}

void big_integer::div_uint(const uint32_t& rhs) {
  uint64_t carry = 0;
  for (size_t i = length(); i > 0; i--) {
    uint64_t cur = _data[i - 1] + carry * BASE;
    _data[i - 1] = static_cast<uint32_t>(cur / rhs);
    carry = cur % rhs;
  }
  trim();
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
  return out << to_string(a);
}

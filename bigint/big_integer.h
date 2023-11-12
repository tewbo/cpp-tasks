#pragma once

#include <iosfwd>
#include <string>
#include <vector>

struct big_integer {
  big_integer();

  big_integer(const big_integer& other);

  big_integer(int);

  big_integer(unsigned);

  big_integer(long a);

  big_integer(unsigned long a);

  big_integer(long long a);

  big_integer(unsigned long long a);

  explicit big_integer(const std::string& str);

  ~big_integer();

  big_integer& operator=(const big_integer& other);

  big_integer& operator+=(const big_integer& rhs);

  big_integer& operator-=(const big_integer& rhs);

  big_integer& operator*=(const big_integer& rhs);

  big_integer& operator/=(const big_integer& rhs);

  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator&=(const big_integer& rhs);

  big_integer& operator|=(const big_integer& rhs);

  big_integer& operator^=(const big_integer& rhs);

  big_integer& operator<<=(int rhs);

  big_integer& operator>>=(int rhs);

  big_integer operator+() const;

  big_integer operator-() const;

  big_integer operator~() const;

  big_integer& operator++();

  big_integer operator++(int);

  big_integer& operator--();

  big_integer operator--(int);

  friend bool operator==(const big_integer& a, const big_integer& b);

  friend bool operator!=(const big_integer& a, const big_integer& b);

  friend bool operator<(const big_integer& a, const big_integer& b);

  friend bool operator>(const big_integer& a, const big_integer& b);

  friend bool operator<=(const big_integer& a, const big_integer& b);

  friend bool operator>=(const big_integer& a, const big_integer& b);

  friend std::string to_string(const big_integer& a);
  friend void swap(big_integer& a, big_integer& b);

private:
  static bool sub_in_pos(big_integer& lhs, const big_integer& rhs, size_t pos);

  template <bool ignore_last_carry>
  static void add_in_pos(big_integer& lhs, const big_integer& rhs, size_t pos);

  template <bool ignore_sign>
  void add_with_ignore(const big_integer& rhs);

  template <bool ignore_sign>
  void sub_with_ignore(const big_integer& rhs);

  bool abs_great_or_eq(const big_integer& rhs) const;

  void div_uint(const uint32_t& rhs);

  friend big_integer mul_uint(const big_integer& a, const uint32_t& b);

  uint32_t operator[](size_t index) const;
  uint32_t& operator[](size_t index);

  static void bit_negation(big_integer& a);

  template <bool return_remainder>
  big_integer& abstract_division(const big_integer& rhs);

  static big_integer& shift_left(big_integer& a, size_t shift);

  template <class Operation>
  void bit_operation(const big_integer& rhs);

  size_t length() const;

  void trim();

  bool eq_zero() const;
  template <bool return_reminder>
  big_integer& divide_with_reminder(const big_integer& rhs);

private:
  std::vector<uint32_t> _data;
  bool _sign{false}; // true for negatives
  big_integer& abstract_division(const big_integer& rhs);
};

big_integer operator+(const big_integer& a, const big_integer& b);

big_integer operator-(const big_integer& a, const big_integer& b);

big_integer operator*(const big_integer& a, const big_integer& b);

big_integer operator/(const big_integer& a, const big_integer& b);

big_integer operator%(const big_integer& a, const big_integer& b);

big_integer operator&(const big_integer& a, const big_integer& b);

big_integer operator|(const big_integer& a, const big_integer& b);

big_integer operator^(const big_integer& a, const big_integer& b);

big_integer operator<<(const big_integer& a, int b);

big_integer operator>>(const big_integer& a, int b);

bool operator==(const big_integer& a, const big_integer& b);

bool operator!=(const big_integer& a, const big_integer& b);

bool operator<(const big_integer& a, const big_integer& b);

bool operator>(const big_integer& a, const big_integer& b);

bool operator<=(const big_integer& a, const big_integer& b);

bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(const big_integer& a);

std::ostream& operator<<(std::ostream& out, const big_integer& a);

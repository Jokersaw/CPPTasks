#pragma once

#include <algorithm>
#include <iosfwd>
#include <iostream>
#include <string>
#include <vector>

struct big_integer {
  big_integer();

  big_integer(const big_integer& other);

  big_integer(unsigned long long a);

  big_integer(long long a);

  big_integer(long a) : big_integer(static_cast<long long>(a)) {}

  big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

  big_integer(int a) : big_integer(static_cast<long long>(a)) {}

  big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

  explicit big_integer(const std::string& str);

  ~big_integer();

private:
  big_integer bigDivision(const big_integer& rhs);

  void trim();

  bool isZero() const;

  void zeroResult();

  void stretch(size_t size);

  static uint32_t singleTwoAddition(uint32_t digit, uint32_t& carry);

  template <class BitWiseOperation>
  void applyBitWiseOp(const big_integer& rhs, BitWiseOperation op) {
    stretch(rhs._data.size());
    uint32_t carry = 1, rhs_carry = 1;
    for (size_t i = 0; i < _data.size(); i++) {
      uint32_t res = _sign ? singleTwoAddition(_data[i], carry) : _data[i];
      uint32_t rhs_res =
          i < rhs._data.size() ? rhs._sign ? singleTwoAddition(rhs._data[i], rhs_carry) : rhs._data[i] : 0;
      _data[i] = op(res, rhs_res);
    }
    _sign = op(_sign, rhs._sign);
    carry = 1;
    if (_sign) {
      for (size_t i = 0; i < _data.size(); i++) {
        _data[i] = singleTwoAddition(_data[i], carry);
      }
    }
    trim();
  }

  uint32_t singleWordDiv(uint32_t b);

  void sumAbs(const big_integer& b);

  void sumDigitAbs(uint32_t b);

  void subAbs(big_integer& res, const big_integer& b) const;

  void subDigitAbs(uint32_t b);

  void mulAbs(const big_integer& b);

  void mulDigitAbs(uint32_t b);

  bool compareLessAbs(const big_integer& other) const;

public:
  void swap(big_integer& other);

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

private:
  std::vector<uint32_t> _data;
  bool _sign;
  static const uint64_t base = 4294967296;
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

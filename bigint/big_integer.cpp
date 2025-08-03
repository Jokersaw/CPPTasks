#include "big_integer.h"

#include <algorithm>
#include <charconv>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <ostream>
#include <stdexcept>

big_integer::big_integer() : _sign(false) {}

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(unsigned long long a) : _sign(false) {
  do {
    _data.push_back(a % base);
    a /= base;
  } while (a != 0);
  trim();
}

big_integer::big_integer(long long a) {
  uint64_t value;
  if (a < 0) {
    _sign = true;
    if (a == LONG_LONG_MIN) {
      value = static_cast<uint64_t>(std::abs(a + 1)) + 1;
    } else {
      value = -a;
    }
  } else {
    _sign = false;
    value = a;
  }
  do {
    _data.push_back(value % base);
    value /= base;
  } while (value != 0);
  trim();
}

big_integer::big_integer(const std::string& str) {
  if (str.empty()) {
    throw std::invalid_argument("Invalid argument: non-empty string expected");
  }
  _sign = false;
  if (str[0] == '-') {
    _sign = true;
    if (str.size() == 1) {
      throw std::invalid_argument("Invalid argument: no digits after unary operation");
    }
  }

  for (size_t i = _sign; i < str.size(); i += 9) {
    size_t next = std::min(static_cast<size_t>(9), str.size() - i);
    uint32_t value = 0;
    if (std::from_chars(str.c_str() + i, str.c_str() + i + next, value).ptr != str.c_str() + i + next) {
      throw std::invalid_argument("Invalid argument: only digits expected");
    }

    if (next < 9) {
      mulDigitAbs(static_cast<uint32_t>(std::pow(10, next)));
    } else {
      mulDigitAbs(1000000000);
    }
    sumDigitAbs(value);
  }
  trim();
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(const big_integer& other) {
  if (&other != this) {
    big_integer(other).swap(*this);
  }
  return *this;
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  if (_sign == rhs._sign) {
    sumAbs(rhs);
    return *this;
  } else {
    if (compareLessAbs(rhs)) {
      _sign = rhs._sign;
      rhs.subAbs(*this, *this);
    } else {
      subAbs(*this, rhs);
    }
    zeroResult();
    return *this;
  }
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  _sign = !_sign;
  *this += rhs;
  _sign = !_sign;
  zeroResult();
  return *this;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  if (rhs.isZero() || isZero()) {
    _data.clear();
    _sign = false;
    return *this;
  }
  mulAbs(rhs);
  _sign = _sign ^ rhs._sign;
  zeroResult();
  return *this;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  bigDivision(rhs);
  zeroResult();
  return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  bigDivision(rhs).swap(*this);
  zeroResult();
  return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  applyBitWiseOp(rhs, std::bit_and());
  zeroResult();
  return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  applyBitWiseOp(rhs, std::bit_or());
  zeroResult();
  return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  applyBitWiseOp(rhs, std::bit_xor());
  zeroResult();
  return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
  _data.insert(_data.begin(), rhs / 32, 0);
  mulDigitAbs(static_cast<uint32_t>(1) << (rhs % 32));
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  if (rhs / 32 >= _data.size()) {
    _data.clear();
    _sign = false;
    return *this;
  }
  _data.erase(_data.begin(), _data.begin() + rhs / 32);
  singleWordDiv(static_cast<uint32_t>(1) << (rhs % 32));
  if (_sign) {
    --(*this);
  }
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  if (!isZero()) {
    big_integer tmp(*this);
    tmp._sign = !tmp._sign;
    return tmp;
  }
  return *this;
}

big_integer big_integer::operator~() const {
  big_integer tmp(*this);
  ++tmp;
  tmp._sign = !tmp._sign;
  return tmp;
}

big_integer& big_integer::operator++() {
  if (_sign) {
    subDigitAbs(1);
  } else {
    sumDigitAbs(1);
  }
  zeroResult();
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer tmp(*this);
  ++(*this);
  return tmp;
}

big_integer& big_integer::operator--() {
  if (_sign) {
    sumDigitAbs(1);
  } else {
    if (isZero()) {
      _sign = true;
      _data.push_back(1);
    } else {
      subDigitAbs(1);
    }
  }
  zeroResult();
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer tmp(*this);
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
  return a._sign == b._sign && a._data == b._data;
}

bool operator!=(const big_integer& a, const big_integer& b) = default;

bool operator<(const big_integer& a, const big_integer& b) {
  if (a._sign != b._sign) {
    return a._sign;
  } else {
    if (!a._sign) {
      return a.compareLessAbs(b);
    } else {
      return b.compareLessAbs(a);
    }
  }
}

bool operator>(const big_integer& a, const big_integer& b) {
  return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}

std::string to_string(const big_integer& a) {
  if (a.isZero()) {
    return "0";
  }
  std::string result;
  big_integer tmp(a);
  while (!tmp.isZero()) {
    uint32_t rem = tmp.singleWordDiv(1000000000);
    size_t len = 9;
    while (rem != 0) {
      result += (rem % 10 + '0');
      rem /= 10;
      len--;
    }
    if (!tmp.isZero()) {
      result.insert(result.end(), len, '0');
    }
  }
  if (a._sign) {
    result += "-";
  }
  std::reverse(result.begin(), result.end());
  return result;
}

big_integer big_integer::bigDivision(const big_integer& rhs) {
  if (rhs.isZero()) {
    throw std::runtime_error("Runtime error: division by zero");
  }

  if (this->compareLessAbs(rhs)) {
    big_integer tmp;
    swap(tmp);
    return tmp;
  }
  big_integer b(rhs);
  bool save_sign = _sign;
  b._sign = false;
  _sign = false;
  int k = 0;
  while (b._data.back() < base / 2) {
    b <<= 1;
    k++;
  }
  (*this) <<= k;
  size_t m = _data.size() - rhs._data.size();
  std::vector<uint32_t> save_b_data = b._data;
  b._data.insert(b._data.begin(), m, 0);
  std::vector<uint32_t> res(m + 1, 0);
  if (*this >= b) {
    res[m] = 1;
    *this -= b;
  }
  for (std::ptrdiff_t i = m - 1; i >= 0; i--) {
    b._data.erase(b._data.begin());
    uint64_t div;
    if (save_b_data.size() + i - 1 >= _data.size()) {
      div = 0;
    } else if (save_b_data.size() + i >= _data.size()) {
      div = _data[save_b_data.size() + i - 1] / save_b_data.back();
    } else {
      div = (_data[save_b_data.size() + i] * base + _data[save_b_data.size() + i - 1]) / save_b_data.back();
    }
    res[i] = std::min(div, base - 1);
    *this -= b * res[i];
    while (*this < 0) {
      res[i]--;
      *this += b;
    }
  }
  trim();
  singleWordDiv(static_cast<uint32_t>(1) << k);
  big_integer rem;
  swap(rem);
  rem._sign = save_sign;
  std::swap(_data, res);
  trim();
  _sign = save_sign ^ rhs._sign;
  return rem;
}

void big_integer::mulDigitAbs(uint32_t b) {
  _data.insert(_data.begin(), 1, 0);

  for (size_t i = 0; i < _data.size() - 1; i++) {
    uint64_t cur = _data[i] + static_cast<uint64_t>(_data[i + 1]) * b;
    _data[i] = static_cast<uint32_t>(cur % base);
    _data[i + 1] = static_cast<uint32_t>(cur / base);
  }
  trim();
}

void big_integer::mulAbs(const big_integer& b) {
  size_t save_size = _data.size();
  _data.insert(_data.begin(), b._data.size(), 0);

  for (size_t i = 0; i < save_size; i++) {
    uint32_t carry = 0;
    for (size_t j = 0; j < b._data.size(); j++) {
      uint64_t cur = _data[i + j] + static_cast<uint64_t>(_data[i + b._data.size()]) * b._data[j] + carry;
      _data[i + j] = static_cast<uint32_t>(cur % base);
      carry = static_cast<uint32_t>(cur / base);
    }
    _data[i + b._data.size()] = carry;
  }
  trim();
}

void big_integer::subDigitAbs(uint32_t b) {
  uint32_t carry_flag = b;

  for (int i = 0; i < _data.size(); ++i) {
    int64_t value = static_cast<int64_t>(_data[i]) - carry_flag;
    if (value < 0) {
      value += base;
      carry_flag = true;
    } else {
      carry_flag = false;
    }
    _data[i] = value;
  }
  trim();
}

void big_integer::subAbs(big_integer& res, const big_integer& b) const {
  bool carry_flag = false;

  int maxSize = std::max(_data.size(), b._data.size());
  res.stretch(maxSize);

  for (int i = 0; i < maxSize; ++i) {
    int64_t value = static_cast<int64_t>(_data[i]) - (i < b._data.size() ? b._data[i] : 0) - carry_flag;
    if (value < 0) {
      value += base;
      carry_flag = true;
    } else {
      carry_flag = false;
    }
    res._data[i] = value;
  }
  res.trim();
}

void big_integer::sumDigitAbs(uint32_t b) {
  uint32_t flag_over = b;
  for (int i = 0; i < _data.size(); ++i) {
    uint64_t value = static_cast<uint64_t>(_data[i]) + flag_over;
    flag_over = value >= base;
    if (flag_over) {
      value -= base;
    }
    _data[i] = value;
  }
  if (flag_over) {
    _data.push_back(flag_over);
  }
}

void big_integer::sumAbs(const big_integer& b) {
  bool flag_over = false;

  stretch(b._data.size());

  for (int i = 0; i < _data.size(); ++i) {
    uint64_t value = static_cast<uint64_t>(_data[i]) + (i < b._data.size() ? b._data[i] : 0) + flag_over;
    flag_over = value >= base;
    if (flag_over) {
      value -= base;
    }
    _data[i] = value;
  }
  if (flag_over) {
    _data.push_back(1);
  }
}

uint32_t big_integer::singleWordDiv(uint32_t b) {
  if (b == 0) {
    throw std::runtime_error("Runtime error: division by zero");
  }
  uint32_t carry = 0;
  for (std::ptrdiff_t i = _data.size() - 1; i >= 0; i--) {
    uint64_t cur = _data[i] + carry * base;
    _data[i] = static_cast<uint32_t>(cur / b);
    carry = static_cast<uint32_t>(cur % b);
  }
  trim();
  if (isZero()) {
    _sign = false;
  }
  return carry;
}

void big_integer::trim() {
  while (!_data.empty() && _data.back() == 0) {
    _data.pop_back();
  }
}

bool big_integer::isZero() const {
  return _data.empty();
}

void big_integer::zeroResult() {
  if (isZero()) {
    _sign = false;
  }
}

void big_integer::swap(big_integer& other) {
  std::swap(_data, other._data);
  std::swap(_sign, other._sign);
}

void big_integer::stretch(size_t size) {
  if (size > _data.size()) {
    _data.resize(size, 0);
  }
}

uint32_t big_integer::singleTwoAddition(uint32_t digit, uint32_t& carry) {
  uint64_t tmp = ~digit + static_cast<uint64_t>(carry);
  carry = static_cast<uint32_t>(tmp / base);
  return static_cast<uint32_t>(tmp % base);
}

bool big_integer::compareLessAbs(const big_integer& other) const {
  if (isZero() && other.isZero()) {
    return false;
  }

  if (_data.size() != other._data.size()) {
    return _data.size() < other._data.size();
  }

  return std::lexicographical_compare(_data.rbegin(), _data.rend(), other._data.rbegin(), other._data.rend());
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
  return out << to_string(a);
}

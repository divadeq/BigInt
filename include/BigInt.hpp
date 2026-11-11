#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <ostream>
#include <string_view>
#include <stdexcept>
#include <bit>
#include <iomanip>

template<std::size_t Bits>
class BigInt {
    static_assert(Bits > 0 && Bits % 64 == 0, "Bits must be a multiple of 64");
    static constexpr std::size_t Limbs = Bits / 64;

public:
    BigInt() = default;

    explicit BigInt(std::uint64_t val) {
        limbs[0] = val;
    }

    explicit BigInt(std::string_view hex_str) {
        if (hex_str.size() >= 2 && hex_str[0] == '0' && (hex_str[1] == 'x' || hex_str[1] == 'X')) {
            hex_str.remove_prefix(2);
        }

        std::size_t bit_offset = 0;
        std::size_t limb_index = 0;
        for (std::size_t i = hex_str.size(); i > 0; --i) {
            char c = hex_str[i - 1];
            std::uint64_t val = 0;

            if (c >= '0' && c <= '9') {
                val = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                val = c - 'A' + 10;
            } else {
                throw std::invalid_argument("Invalid argument");
            }

            if (limb_index >= Limbs) {
                throw std::overflow_error("Overflow");
            }

            limbs[limb_index] |= (val << bit_offset);

            bit_offset += 4;

            if (bit_offset == 64) {
                bit_offset = 0;
                limb_index++;
            }
        }
    }

    BigInt& operator+=(const BigInt& other) noexcept {
        std::uint64_t carry = 0;

        for (std::size_t i = 0; i < Limbs; ++i) {
            unsigned __int128 sum = static_cast<unsigned __int128>(this->limbs[i]) + other.limbs[i] + carry;
            this->limbs[i] = static_cast<std::uint64_t>(sum);
            carry = static_cast<std::uint64_t>(sum >> 64);
        }

        return *this;
    }

    BigInt& operator-=(const BigInt& other) noexcept {
        std::uint64_t borrow = 0;

        for (std::size_t i = 0; i < Limbs; ++i) {
            unsigned __int128 diff = static_cast<unsigned __int128>(this->limbs[i]) - other.limbs[i] - borrow;
            this->limbs[i] = static_cast<std::uint64_t>(diff);
            borrow = (diff >> 64) ? 1 : 0;
        }

        return *this;
    }

    BigInt& operator*=(const BigInt& other) noexcept {
        std::array<std::uint64_t, Limbs> temp_result{};

        for (std::size_t i = 0; i < Limbs; ++i) {
            if (this->limbs[i] == 0) continue;

            unsigned __int128 carry = 0;
            for (std::size_t j = 0; i + j < Limbs; ++j) {
                unsigned __int128 sum = static_cast<unsigned __int128>(temp_result[i + j]) +
                                        static_cast<unsigned __int128>(this->limbs[i]) * other.limbs[j] +
                                        carry;

                temp_result[i + j] = static_cast<std::uint64_t>(sum);
                carry = sum >> 64;
            }
        }
        this->limbs = temp_result;
        return *this;
    }

    BigInt& operator/=(const BigInt& other) {
        BigInt quotient, remainder;
        this->divide_modulo(other, quotient, remainder);
        *this = quotient;
        return *this;
    }

    BigInt& operator%=(const BigInt& other) {
        BigInt quotient, remainder;
        this->divide_modulo(other, quotient, remainder);
        *this = remainder;
        return *this;
    }

    [[nodiscard]] BigInt operator+(const BigInt& other) const noexcept {
        BigInt result = *this;
        result += other;
        return result;
    }

    [[nodiscard]] BigInt operator-(const BigInt& other) const noexcept {
        BigInt result = *this;
        result -= other;
        return result;
    }

    [[nodiscard]] BigInt operator*(const BigInt& other) const noexcept {
        BigInt result = *this;
        result *= other;
        return result;
    }

    [[nodiscard]] BigInt operator/(const BigInt& other) const {
        BigInt result = *this;
        result /= other;
        return result;
    }

    [[nodiscard]] BigInt operator%(const BigInt& other) const {
        BigInt result = *this;
        result %= other;
        return result;
    }

    BigInt& operator<<=(std::size_t shift) noexcept {
        if (shift >= Bits) {
            limbs.fill(0);
            return *this;
        }

        if (shift == 0) return *this;

        std::size_t limb_shift = shift / 64;
        std::size_t bit_shift = shift % 64;

        if (bit_shift == 0) {
            for (std::size_t i = Limbs; i > 0; --i) {
                std::size_t idx = i - 1;
                if (idx >= limb_shift) {
                    limbs[idx] = limbs[idx - limb_shift];
                } else {
                    limbs[idx] = 0;
                }
            }
        } else {
            for (std::size_t i = Limbs; i > 0; --i) {
                std::size_t idx = i - 1;

                std::uint64_t low = (idx >= limb_shift) ? limbs[idx - limb_shift] : 0;
                std::uint64_t high = (idx > limb_shift) ? limbs[idx - limb_shift - 1] : 0;

                limbs[idx] = (low << bit_shift) | (high >> (64 - bit_shift));
            }
        }

        return *this;
    }

    [[nodiscard]] BigInt operator<<(std::size_t shift) const noexcept {
        BigInt result = *this;
        result <<= shift;
        return result;
    }

    BigInt& operator>>=(std::size_t shift) noexcept {
        if (shift >= Bits) {
            limbs.fill(0);
            return *this;
        }

        if (shift == 0) return *this;

        std::size_t limb_shift = shift / 64;
        std::size_t bit_shift = shift % 64;

        if (bit_shift == 0) {
            for (std::size_t i = 0; i < Limbs; ++i) {
                if (i + limb_shift < Limbs) {
                    limbs[i] = limbs[i + limb_shift];
                } else {
                    limbs[i] = 0;
                }
            }
        } else {
            for (std::size_t i = 0; i < Limbs; ++i) {
                std::uint64_t low = (i + limb_shift < Limbs) ? limbs[i + limb_shift] : 0;
                std::uint64_t high = (i + limb_shift + 1 < Limbs) ? limbs[i + limb_shift + 1] : 0;

                limbs[i] = (low >> bit_shift) | (high << (64 - bit_shift));
            }
        }

        return *this;
    }

    [[nodiscard]] BigInt operator>>(std::size_t shift) const noexcept {
        BigInt result = *this;
        result >>= shift;
        return result;
    }

    BigInt& operator&=(const BigInt& other) noexcept {
        for (std::size_t i = 0; i < Limbs; ++i) {
            this->limbs[i] &= other.limbs[i];
        }
        return *this;
    }

    BigInt& operator|=(const BigInt& other) noexcept {
        for (std::size_t i = 0; i < Limbs; ++i) {
            this->limbs[i] |= other.limbs[i];
        }
        return *this;
    }

    BigInt& operator^=(const BigInt& other) noexcept {
        for (std::size_t i = 0; i < Limbs; ++i) {
            this->limbs[i] ^= other.limbs[i];
        }
        return *this;
    }

    [[nodiscard]] BigInt operator~() const noexcept {
        BigInt result;
        for (std::size_t i = 0; i < Limbs; ++i) {
            result.limbs[i] = ~this->limbs[i];
        }
        return result;
    }

    [[nodiscard]] bool operator>(const BigInt& other) const noexcept {
        for (std::size_t i = Limbs; i > 0; --i) {
            std::size_t idx = i - 1;
            if (this->limbs[idx] > other.limbs[idx]) return true;
            if (this->limbs[idx] < other.limbs[idx]) return false;
        }
        return false;
    }

    [[nodiscard]] bool operator<(const BigInt& other) const noexcept {
        for (std::size_t i = Limbs; i > 0; --i) {
            std::size_t idx = i - 1;
            if (this->limbs[idx] < other.limbs[idx]) return true;
            if (this->limbs[idx] > other.limbs[idx]) return false;
        }
        return false;
    }

    [[nodiscard]] bool operator<=(const BigInt& other) const noexcept {
        return !(*this > other);
    }

    [[nodiscard]] bool operator>=(const BigInt& other) const noexcept {
        return !(*this < other);
    }

    [[nodiscard]] bool operator==(const BigInt& other) const noexcept {
        return this->limbs == other.limbs;
    }

    [[nodiscard]] bool operator!=(const BigInt& other) const noexcept {
        return this->limbs != other.limbs;
    }

    [[nodiscard]] std::size_t bit_length() const noexcept {
        for (std::size_t i = Limbs; i > 0; --i) {
            std::size_t idx = i - 1;
            if (limbs[idx] != 0) {
                return idx * 64 + (64 - std::countl_zero(limbs[idx]));
            }
        }
        return 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const BigInt& b) {
        os << "0x";

        bool started_printing = false;

        for (std::size_t i = Limbs; i > 0; --i) {
            std::size_t idx = i - 1;
            std::uint64_t val = b.limbs[idx];

            if (started_printing) {
                os << std::setw(16) << std::setfill('0') << std::hex << val;
            } else if (val != 0) {
                os << std::hex << val;
                started_printing = true;
            }
        }

        if (!started_printing) {
            os << "0";
        }

        os << std::dec;

        return os;
    }

private:
    std::array<std::uint64_t, Limbs> limbs{};

    void divide_modulo(const BigInt& divisor, BigInt& quotient, BigInt& remainder) const {
        if (divisor == BigInt(0)) {
            throw std::invalid_argument("Division by zero");
        }

        if (*this < divisor) {
            quotient = BigInt(0);
            remainder = *this;
            return;
        }

        quotient = BigInt(0);
        remainder = BigInt(0);

        std::size_t total_bits = this->bit_length();
        for (std::size_t i = total_bits; i > 0; --i) {
            std::size_t bit_idx = i - 1;

            remainder <<= 1;

            std::size_t limb_idx = bit_idx / 64;
            std::size_t bit_off = bit_idx % 64;

            if ((this->limbs[limb_idx] >> bit_off) & 1) {
                remainder.limbs[0] |= 1;
            }

            if (remainder >= divisor) {
                remainder -= divisor;
                quotient.limbs[limb_idx] |= (1ULL << bit_off);
            }
        }
    }
};

using BigInt256 = BigInt<256>;
using BigInt512 = BigInt<512>;
using BigInt1024 = BigInt<1024>;
using BigInt2048 = BigInt<2048>;
using BigInt4096 = BigInt<4096>;
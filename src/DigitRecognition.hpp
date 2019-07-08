#pragma once

#include<opencv2/core.hpp>
#include<array>
#include<string>

int constexpr FACTOR=1; // TODO currently code is not correct if FACTOR is not 1
int constexpr HEIGHT=12,WIDTH=10;
template<class T>
using Digit_=cv::Matx<T,HEIGHT*FACTOR,WIDTH*FACTOR>;

int constexpr NDIGIT=16;
extern std::array<Digit_<int8_t>,NDIGIT> const digits;
extern std::array<Digit_<float>,NDIGIT> const digits_w;

char const* const name="0123456789ABCDEF";
inline int indexOf(char c){
	if('0'<=c&&c<='9')
		return c-'0';
	else if('A'<=c&&c<='F')
		return c-'A'+10;
	else return -1;
}

/// Get image of a string of digits.
cv::Mat_<int8_t> getDigitMat(std::string);

/// Return the most likely index.
int recognize(Digit_<float>);

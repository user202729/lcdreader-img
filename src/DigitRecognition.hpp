#pragma once

#include<opencv2/core.hpp>
#include<array>
#include<string>

int constexpr FACTOR=1; // TODO currently code is not correct if FACTOR is not 1
int constexpr HEIGHT=12,WIDTH=10;
template<class T>
using Digit_=cv::Matx<T,HEIGHT*FACTOR,WIDTH*FACTOR>;

int constexpr NDIGIT=16;

/// Digit index -> Matx. 0 is background, 1 is foreground.
extern std::array<Digit_<uint8_t>,NDIGIT> const digits;

char const* const name="0123456789ABCDEF";
inline int indexOf(char c){
	if('0'<=c&&c<='9')
		return c-'0';
	else if('A'<=c&&c<='F')
		return c-'A'+10;
	else return -1;
}

/// Get image of a string of digits. Same format as 'digits'.
cv::Mat_<uint8_t> getDigitMat(std::string);

/// Return the most likely index, assume foreground color is larger.
/// Old algorithm.
int recognize(Digit_<float>);

/// Old algorithm.
std::string recognizeDigits(cv::Mat m);

struct RecognitionResult{
	char digit;
	double bestScore; // lower is better
	double centerScore; // of the recognized digit at the center. >= bestScore
};
RecognitionResult recognizeDigitTemplateMatchingExtended(cv::Mat_<uint8_t> const& m, double zoomFactor);
char recognizeDigitTemplateMatching(cv::Mat_<uint8_t> const& m, double zoomFactor);

// return colRanges from the input matrix. Each resulting digit is surrounded by border.
std::array<cv::Mat, 4> iterateDigits(cv::Mat m, double zoomFactor, double border);

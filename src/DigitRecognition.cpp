#include"DigitRecognition.hpp"
#include<iostream>
#include<limits>
#include<cassert>
#include<opencv2/imgproc.hpp>

std::array<Digit_<uint8_t>,NDIGIT> const digits=[](){
	std::array<Digit_<uint8_t>,NDIGIT> digits;
	char const* data[HEIGHT]={
	"   #####       ##        #####     ######         ##     ########    ######   #########    #####      #####       ##     ########      ##### # #######    ########## ##########",
	"  ##   ##    ####       ##   ##   ###   ##       ###     #######    ##   ###  #########   ##   ##    ##   ##      ##      ##    ##    ##   ###  ##   ##    ##     ##  ##     ##",
	"  #     #      ##      ##     ##  ##     ##     # ##     #         ##     ##  #       #  ##     ##  ##     ##    # ##     ##     ##  ##     ##  ##    ##   ##      #  ##      #",
	" ##     ##     ##      ##     ##         ##     # ##     #         ##         #     ##   ##     ##  ##     ##    # ##     ##     ##  ##      #  ##    ##   ##   #     ##   #   ",
	" ##     ##     ##             ##        ##     #  ##     ######    ## ####         ##    ###    ##  ##     ##    # ##     ##    ##  ##          ##     ##  ##   #     ##   #   ",
	" ##     ##     ##            ##     #####      #  ##    ##    ##   ###   ##        ##     #######   ##     ##   #   ##    #######   ##          ##     ##  ######     ######   ",
	" ##     ##     ##          ###          ##    #   ##    #      ##  ##     ##      ##      #######    ##   ###   #   ##    ##    ##  ##          ##     ##  ##   #     ##   #   ",
	" ##     ##     ##         ##             ##   #   ##           ##  ##     ##      ##     ##    ###    #### ##   ######    ##     ## ##          ##     ##  ##   #     ##   #   ",
	" ##     ##     ##        ##              ##  #########         ##  ##     ##     ##      ##     ##         ##  #     ##   ##     ##  ##      #  ##    ##   ##         ##       ",
	"  #     #      ##       ##     #  ##     ##       ##    ##     ##  ##     ##     ##      ##     ##  ##     ##  #     ##   ##     ##  ##      #  ##    ##   ##      #  ##       ",
	"  ##   ##      ##      #########  ###   ##        ##    ###   ##    ##   ##      ##       ##   ##   ###   ##   #     ##   ##    ##    ##    #   ##   ##    ##     ##  ##       ",
	"   #####     ######    #########   ######        ####    ######      #####       ##        #####     ######   ###   #### ########      #####   #######    ########## #####     ",
	};
	for(int rowx=0;rowx<HEIGHT;++rowx){
		char const* row=data[rowx];
		for(int i=0;i<NDIGIT;++i){
			int base=i*(WIDTH+1);
			auto& digit=digits[i];
			for(int colx=0;colx<WIDTH;++colx){
				assert(row[colx]=='#'||row[colx]==' ');
				digit(rowx,colx)=row[base+colx]=='#'?1:0;
			}
			if(i==NDIGIT-1)
				assert(row[base+WIDTH]==0);
			else
				assert(row[base+WIDTH]==' ');
		}
	} // "simple" parser
	return digits;
}();

std::array<Digit_<float>,NDIGIT> const digits_w=[](){
	std::array<Digit_<float>,NDIGIT> digits_w;
	for(int i=0;i<NDIGIT;++i){
		auto const& digit=digits[i];
		auto& digit_w=digits_w[i];

		float sum=cv::sum(digit)[0];
		int const DIGITSIZE=WIDTH*HEIGHT;

		// normalize -> sum = 0
		cv::subtract(digit,sum/DIGITSIZE,digit_w);

		// now, max possible value of dot with a binary matrix is (1-sum/DIGITSIZE)*sum
		float maxscore=(1-sum/DIGITSIZE)*sum;
		cv::divide(digit_w,maxscore,digit_w);

		// blur (however the identities above will be only approx correct)
		// cv::blur(digit_w,digit_w,
		// 		{2,2}, // kernel size
		// 		cv::Point(-1,-1),cv::BORDER_CONSTANT);
	}
	return digits_w;
}();

cv::Mat_<uint8_t> getDigitMat(std::string s){
	int constexpr GAP=1;
	cv::Mat_<uint8_t> ans(HEIGHT,(WIDTH+GAP)*s.size()-GAP,(uint8_t)0);
	int x=0;
	for(char c:s){
		cv::Mat(digits[indexOf(c)],
				false // copyData
			   ).copyTo(ans.colRange(x,x+WIDTH));
		x+=WIDTH+GAP;
	}
	return ans;
}

/*
int validateIndexOf();
static int validateIndexOf_int=validateIndexOf();
int validateIndexOf(){
	(void)validateIndexOf_int;
	for(int i=0;i<NDIGIT;++i)
		assert(indexOf(name[i])==i);
	return 0;
}
*/

int recognize(Digit_<float> m){
	int besti=0;
	float bestscore=std::numeric_limits<float>::min();
	for(int i=0;i<NDIGIT;++i){
		float score=m.dot(digits_w[i]);
		//std::cerr<<i<<'\t'<<name[i]<<'\t'<<score<<'\n';
		if(score>bestscore){
			besti=i;
			bestscore=score;
		}
	}
	//std::cerr<<" =>\t"<<besti<<'\t'<<name[besti]<<'\n';
	return besti;
}

std::string recognizeDigits(cv::Mat m){
	// assume m has HEIGHT cols and each digit is separated by GAP col(s)
	int constexpr GAP=1;
	std::string ans((m.cols+GAP)/(WIDTH+GAP),' ');
	int x=0;
	for(char& c:ans){
		c=name[recognize(
				m.colRange(x,x+WIDTH)
				)];
		x+=WIDTH+GAP;
	}
	return ans;
}

RecognitionResult recognizeDigitTemplateMatchingExtended(cv::Mat_<uint8_t> const& image, double zoomFactor){
	double bestScore=DBL_MAX;
	double centerScore;
	int bestIndex;

	cv::Mat_<uint8_t> template_;
	cv::Mat_<float> curResult;
	for(int index=0; index<NDIGIT; ++index){
		cv::resize(digits[index], template_, {}, zoomFactor, zoomFactor, cv::INTER_NEAREST);
		cv::matchTemplate(image, template_, curResult, cv::TM_CCOEFF_NORMED);
		double curScore;
		cv::minMaxLoc(curResult, &curScore); // take the minimum because the mask is inverted
		assert(curResult.rows!=-1 and curResult.cols!=-1);

		if(curScore<bestScore){
			bestIndex=index;
			bestScore=curScore;
			centerScore=curResult(curResult.rows/2, curResult.cols/2);
		}
	}

	assert(bestScore<DBL_MAX);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
	return {name[bestIndex], bestScore, centerScore};
#pragma GCC diagnostic pop
}

char recognizeDigitTemplateMatching(cv::Mat_<uint8_t> const& image, double zoomFactor){
	return recognizeDigitTemplateMatchingExtended(image, zoomFactor).digit;
}

std::array<cv::Mat, 4> iterateDigits(cv::Mat image, double zoomFactor, double border){
	assert(image.cols==int((WIDTH*4+3+border*2)*zoomFactor));
	assert(image.rows==int((HEIGHT+border*2)*zoomFactor));

	std::array<cv::Mat, 4> result;
	for(int i=0; i<4;++i){
		auto const offset=(WIDTH+1)*i;
		result[i]=image.colRange(offset*zoomFactor,(offset+border*2+WIDTH)*zoomFactor);
	}
	return result;
}

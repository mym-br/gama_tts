/***************************************************************************
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
// 2014-09
// This file was copied from Gnuspeech and modified by Marcelo Y. Matuda.

#ifndef VTM_WAVETABLE_GLOTTAL_SOURCE_FIR_FILTER_H_
#define VTM_WAVETABLE_GLOTTAL_SOURCE_FIR_FILTER_H_

#include <cmath>
#include <vector>

#include "Exception.h"



namespace GS {
namespace VTM {

/******************************************************************************
*
*  class:    WavetableGlottalSourceFIRFilter
*
*  purpose:  Lowpass FIR filter.
*
******************************************************************************/
template<typename FloatType>
class WavetableGlottalSourceFIRFilter {
public:
	WavetableGlottalSourceFIRFilter(FloatType beta, FloatType gamma, FloatType cutoff);
	~WavetableGlottalSourceFIRFilter() = default;

	void reset();
	FloatType filter(FloatType input, int needOutput);
private:
	enum {
		LIMIT = 200
	};

	WavetableGlottalSourceFIRFilter(const WavetableGlottalSourceFIRFilter&) = delete;
	WavetableGlottalSourceFIRFilter& operator=(const WavetableGlottalSourceFIRFilter&) = delete;
	WavetableGlottalSourceFIRFilter(WavetableGlottalSourceFIRFilter&&) = delete;
	WavetableGlottalSourceFIRFilter& operator=(WavetableGlottalSourceFIRFilter&&) = delete;

	static int maximallyFlat(FloatType beta, FloatType gamma, int* np, FloatType* coefficient);
	static void trim(FloatType cutoff, int* numberCoefficients, FloatType* coefficient);
	static int increment(int pointer, int modulus);
	static int decrement(int pointer, int modulus);
	static void rationalApproximation(FloatType number, int* order, int* numerator, int* denominator);

	std::vector<FloatType> data_;
	std::vector<FloatType> coef_;
	int ptr_;
	int numberTaps_;
};



template<typename FloatType>
WavetableGlottalSourceFIRFilter<FloatType>::WavetableGlottalSourceFIRFilter(FloatType beta, FloatType gamma, FloatType cutoff)
{
	int numberCoefficients;
	FloatType coefficient[LIMIT + 1];

	/*  DETERMINE IDEAL LOW PASS FILTER COEFFICIENTS  */
	maximallyFlat(beta, gamma, &numberCoefficients, coefficient);

	/*  TRIM LOW-VALUE COEFFICIENTS  */
	trim(cutoff, &numberCoefficients, coefficient);

	/*  DETERMINE THE NUMBER OF TAPS IN THE FILTER  */
	numberTaps_ = (numberCoefficients * 2) - 1;

	/*  ALLOCATE MEMORY FOR DATA AND COEFFICIENTS  */
	data_.resize(numberTaps_);
	coef_.resize(numberTaps_);

	/*  INITIALIZE THE COEFFICIENTS  */
	int increment = -1;
	int pointer = numberCoefficients;
	for (int i = 0; i < numberTaps_; i++) {
		coef_[i] = coefficient[pointer];
		pointer += increment;
		if (pointer <= 0) {
			pointer = 2;
			increment = 1;
		}
	}

	/*  SET POINTER TO FIRST ELEMENT  */
	ptr_ = 0;

#if 0
	/*  PRINT OUT  */
	printf("\n");
	for (int i = 0; i < numberTaps_; i++) {
		printf("FIR coef_[%-d] = %11.8f\n", i, coef_[i]);
	}
#endif
}

template<typename FloatType>
void
WavetableGlottalSourceFIRFilter<FloatType>::reset()
{
	for (auto& item : data_) item = 0.0;
	ptr_ = 0;
}

/******************************************************************************
*
*  function:  maximallyFlat
*
*  purpose:   Calculates coefficients for a linear phase lowpass FIR
*             filter, with beta being the center frequency of the
*             transition band (as a fraction of the sampling
*             frequency), and gamma the width of the transition
*             band.
*
******************************************************************************/
template<typename FloatType>
int
WavetableGlottalSourceFIRFilter<FloatType>::maximallyFlat(FloatType beta, FloatType gamma, int* np, FloatType* coefficient)
{
	FloatType a[LIMIT + 1], c[LIMIT + 1];
	int numerator;

	/*  INITIALIZE NUMBER OF POINTS  */
	*np = 0;

	/*  CUT-OFF FREQUENCY MUST BE BETWEEN 0 HZ AND NYQUIST  */
	if ((beta <= 0.0f) || (beta >= 0.5f)) {
		THROW_EXCEPTION(VTMException, "Beta out of range.");
	}

	/*  TRANSITION BAND MUST FIT WITH THE STOP BAND  */
	const FloatType betaMinimum = ((2.0f * beta) < (1.0f - 2.0f * beta)) ?
						(2.0f * beta) :
						(1.0f - 2.0f * beta);
	if ((gamma <= 0.0) || (gamma >= betaMinimum)) {
		THROW_EXCEPTION(VTMException, "Gamma out of range.");
	}

	/*  MAKE SURE TRANSITION BAND NOT TOO SMALL  */
	int nt = static_cast<int>(1.0f / (4.0f * gamma * gamma));
	if (nt > 160) {
		THROW_EXCEPTION(VTMException, "Gamma too small.");
	}

	/*  CALCULATE THE RATIONAL APPROXIMATION TO THE CUT-OFF POINT  */
	const FloatType ac = (1.0f + std::cos((2.0f * static_cast<FloatType>(M_PI)) * beta)) / 2.0f;
	rationalApproximation(ac, &nt, &numerator, np);

	/*  CALCULATE FILTER ORDER  */
	const int n = (2 * (*np)) - 1;
	if (numerator == 0) {
		numerator = 1;
	}

	/*  COMPUTE MAGNITUDE AT NP POINTS  */
	c[1] = a[1] = 1.0;
	const int ll = nt - numerator;

	for (int i = 2; i <= *np; i++) {
		c[i] = std::cos((2.0f * static_cast<FloatType>(M_PI)) * (static_cast<FloatType>(i - 1) / n));
		const FloatType x = (1.0f - c[i]) / 2.0f;
		FloatType y = x;

		if (numerator == nt) {
			continue;
		}

		FloatType sum = 1.0;
		for (int j = 1; j <= ll; j++) {
			FloatType z = y;
			if (numerator != 1) {
				for (int jj = 1; jj <= (numerator - 1); jj++) {
					z *= 1.0f + (static_cast<FloatType>(j) / jj);
				}
			}
			y *= x;
			sum += z;
		}
		a[i] = sum * std::pow((1.0f - x), numerator);
	}

	/*  CALCULATE WEIGHTING COEFFICIENTS BY AN N-POINT IDFT  */
	for (int i = 1; i <= *np; i++) {
		coefficient[i] = a[1] / 2.0f;
		for (int j = 2; j <= *np; j++) {
			int m = ((i - 1) * (j - 1)) % n;
			if (m > nt) {
				m = n - m;
			}
			coefficient[i] += c[m+1] * a[j];
		}
		coefficient[i] *= 2.0f / static_cast<FloatType>(n);
	}

	return 0;
}

/******************************************************************************
*
*  function:  trim
*
*  purpose:   Trims the higher order coefficients of the FIR filter
*             which fall below the cutoff value.
*
******************************************************************************/
template<typename FloatType>
void
WavetableGlottalSourceFIRFilter<FloatType>::trim(FloatType cutoff, int* numberCoefficients, FloatType* coefficient)
{
	for (int i = *numberCoefficients; i > 0; i--) {
		if (std::abs(coefficient[i]) >= std::abs(cutoff)) {
			*numberCoefficients = i;
			return;
		}
	}
}

/******************************************************************************
*
*  function:  increment
*
*  purpose:   Increments the pointer to the circular FIR filter
*             buffer, keeping it in the range 0 -> modulus-1.
*
******************************************************************************/
template<typename FloatType>
int
WavetableGlottalSourceFIRFilter<FloatType>::increment(int pointer, int modulus)
{
	if (++pointer >= modulus) {
		return 0;
	} else {
		return pointer;
	}
}

/******************************************************************************
*
*  function:  decrement
*
*  purpose:   Decrements the pointer to the circular FIR filter
*             buffer, keeping it in the range 0 -> modulus-1.
*
******************************************************************************/
template<typename FloatType>
int
WavetableGlottalSourceFIRFilter<FloatType>::decrement(int pointer, int modulus)
{
	if (--pointer < 0) {
		return modulus - 1;
	} else {
		return pointer;
	}
}

template<typename FloatType>
FloatType WavetableGlottalSourceFIRFilter<FloatType>::filter(FloatType input, int needOutput)
{
	if (needOutput) {
		FloatType output{};

		/*  PUT INPUT SAMPLE INTO DATA BUFFER  */
		data_[ptr_] = input;

		/*  SUM THE OUTPUT FROM ALL FILTER TAPS  */
		for (int i = 0; i < numberTaps_; i++) {
			output += data_[ptr_] * coef_[i];
			ptr_ = increment(ptr_, numberTaps_);
		}

		/*  DECREMENT THE DATA POINTER READY FOR NEXT CALL  */
		ptr_ = decrement(ptr_, numberTaps_);

		/*  RETURN THE OUTPUT VALUE  */
		return output;
	} else {
		/*  PUT INPUT SAMPLE INTO DATA BUFFER  */
		data_[ptr_] = input;

		/*  ADJUST THE DATA POINTER, READY FOR NEXT CALL  */
		ptr_ = decrement(ptr_, numberTaps_);

		return 0.0;
	}
}

/******************************************************************************
*
*  function:  rationalApproximation
*
*  purpose:   Calculates the best rational approximation to 'number',
*             given the maximum 'order'.
*
******************************************************************************/
template<typename FloatType>
void
WavetableGlottalSourceFIRFilter<FloatType>::rationalApproximation(FloatType number, int* order, int* numerator, int* denominator)
{
	/*  RETURN IMMEDIATELY IF THE ORDER IS LESS THAN ONE  */
	if (*order <= 0) {
		*numerator = 0;
		*denominator = 0;
		*order = -1;
		return;
	}

	/*  FIND THE ABSOLUTE VALUE OF THE FRACTIONAL PART OF THE NUMBER  */
	const FloatType fractionalPart = std::abs(number - static_cast<int>(number));

	/*  DETERMINE THE MAXIMUM VALUE OF THE DENOMINATOR  */
	int orderMaximum = 2 * (*order);
	orderMaximum = (orderMaximum > LIMIT) ? LIMIT : orderMaximum;

	/*  FIND THE BEST DENOMINATOR VALUE  */
	FloatType minimumError = 1.0;
	int modulus = 0;
	for (int i = (*order); i <= orderMaximum; i++) {
		const FloatType ps = i * fractionalPart;
		int ip = static_cast<int>(ps + 0.5f);
		FloatType error = std::abs((ps - static_cast<double>(ip)) / i);
		if (error < minimumError) {
			minimumError = error;
			modulus = ip;
			*denominator = i;
		}
	}

	/*  DETERMINE THE NUMERATOR VALUE, MAKING IT NEGATIVE IF NECESSARY  */
	*numerator = static_cast<int>(std::abs(number)) * (*denominator) + modulus;
	if (number < 0.0) {
		*numerator *= -1;
	}

	/*  SET THE ORDER  */
	*order = *denominator - 1;

	/*  RESET THE NUMERATOR AND DENOMINATOR IF THEY ARE EQUAL  */
	if (*numerator == *denominator) {
		*denominator = orderMaximum;
		*order = *numerator = *denominator - 1;
	}
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_WAVETABLE_GLOTTAL_SOURCE_FIR_FILTER_H_ */

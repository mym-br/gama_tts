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

#ifndef VTM_SAMPLE_RATE_CONVERTER_H_
#define VTM_SAMPLE_RATE_CONVERTER_H_

#include <cmath>
#include <functional>
#include <vector>



namespace GS {
namespace VTM {

template<typename TFloat>
class SampleRateConverter {
public:
	SampleRateConverter(TFloat inputRate, TFloat outputRate, std::function<void(float)> output);
	~SampleRateConverter() = default;

	void reset();
	void dataFill(TFloat data);
	void flushBuffer();
private:
	enum {
		BUFFER_SIZE = 1024, /*  ring buffer size  */
		L_BITS  = 8,
		L_RANGE = 1 << L_BITS,
		M_BITS  = 8,
		M_RANGE = 1 << M_BITS,
		ZERO_CROSSINGS = 13,
		FILTER_LENGTH  = ZERO_CROSSINGS * L_RANGE,
		FRACTION_BITS  = L_BITS + M_BITS,
		FRACTION_RANGE = 1 << FRACTION_BITS,
		FILTER_LIMIT   = FILTER_LENGTH - 1,
		M_MASK        = 0x000000FF,
		L_MASK        = 0x0000FF00,
		FRACTION_MASK = 0x0000FFFF,
		N_MASK        = 0xFFFF0000
	};

	SampleRateConverter(const SampleRateConverter&) = delete;
	SampleRateConverter& operator=(const SampleRateConverter&) = delete;
	SampleRateConverter(SampleRateConverter&&) = delete;
	SampleRateConverter& operator=(SampleRateConverter&&) = delete;

	void initializeConversion(TFloat inputRate, TFloat outputRate);
	void initializeBuffer();
	void initializeFilter();
	void dataEmpty();

	static TFloat Izero(TFloat x);
	static void srIncrement(int *pointer, int modulus);
	static void srDecrement(int *pointer, int modulus);
	template<typename T> static T nValue(T x)        { return (x & N_MASK) >> FRACTION_BITS; }
	template<typename T> static T lValue(T x)        { return (x & L_MASK) >> M_BITS; }
	template<typename T> static T mValue(T x)        { return x & M_MASK; }
	template<typename T> static T fractionValue(T x) { return x & FRACTION_MASK; }

	TFloat sampleRateRatio_;
	int fillPtr_;
	int emptyPtr_;
	int padSize_;
	int fillSize_;
	unsigned int timeRegisterIncrement_;
	unsigned int filterIncrement_;
	unsigned int phaseIncrement_;
	unsigned int timeRegister_;
	int fillCounter_;

	std::vector<TFloat> h_;
	std::vector<TFloat> deltaH_;
	std::vector<TFloat> buffer_;
	std::function<void(float)> output_;
};



template<typename TFloat>
SampleRateConverter<TFloat>::SampleRateConverter(TFloat inputRate, TFloat outputRate, std::function<void(float)> output)
		: sampleRateRatio_()
		, fillPtr_()
		, emptyPtr_()
		, padSize_()
		, fillSize_()
		, timeRegisterIncrement_()
		, filterIncrement_()
		, phaseIncrement_()
		, timeRegister_()
		, fillCounter_()
		, h_(FILTER_LENGTH)
		, deltaH_(FILTER_LENGTH)
		, buffer_(BUFFER_SIZE)
		, output_(output)
{
	initializeConversion(inputRate, outputRate);
}

template<typename TFloat>
void
SampleRateConverter<TFloat>::reset()
{
	emptyPtr_ = 0;
	timeRegister_ = 0;
	fillCounter_ = 0;
	initializeBuffer();
}

/******************************************************************************
*
*  function:  initializeConversion
*
*  purpose:   Initializes all the sample rate conversion functions.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::initializeConversion(TFloat inputRate, TFloat outputRate)
{
	/*  INITIALIZE FILTER IMPULSE RESPONSE  */
	initializeFilter();

	/*  CALCULATE SAMPLE RATE RATIO  */
	sampleRateRatio_ = outputRate / inputRate;

	/*  CALCULATE TIME REGISTER INCREMENT  */
	timeRegisterIncrement_ = static_cast<unsigned int>(std::rint(std::pow(2.0, FRACTION_BITS) / sampleRateRatio_));

	/*  CALCULATE ROUNDED SAMPLE RATE RATIO  */
	TFloat roundedSampleRateRatio = std::pow(2.0, FRACTION_BITS) / timeRegisterIncrement_;

	/*  CALCULATE PHASE OR FILTER INCREMENT  */
	if (sampleRateRatio_ >= 1.0f) {
		filterIncrement_ = L_RANGE;
	} else {
		phaseIncrement_ = static_cast<unsigned int>(std::rint(sampleRateRatio_ * FRACTION_RANGE));
	}

	/*  CALCULATE PAD SIZE  */
	padSize_ = (sampleRateRatio_ >= 1.0f) ?
				static_cast<int>(ZERO_CROSSINGS) :
				static_cast<int>(ZERO_CROSSINGS / roundedSampleRateRatio) + 1;

	/*  INITIALIZE THE RING BUFFER  */
	initializeBuffer();
}

/******************************************************************************
*
*  function:  Izero
*
*  purpose:   Returns the value for the modified Bessel function of
*             the first kind, order 0, as a double.
*
******************************************************************************/
template<typename TFloat>
TFloat SampleRateConverter<TFloat>::Izero(TFloat x)
{
	const TFloat IzeroEPSILON = 1E-21;

	TFloat sum, u, halfx, temp;
	int n;

	sum = u = n = 1;
	halfx = x / 2.0f;

	do {
		temp = halfx / n;
		n += 1;
		temp *= temp;
		u *= temp;
		sum += u;
	} while (u >= IzeroEPSILON * sum);

	return sum;
}

/******************************************************************************
*
* function:  initializeBuffer
*
*  purpose:  Initializes the ring buffer used for sample rate
*            conversion.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::initializeBuffer()
{
	/*  FILL THE RING BUFFER WITH ALL ZEROS  */
	for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
		buffer_[i] = 0.0;
	}

	/*  INITIALIZE FILL POINTER  */
	fillPtr_ = padSize_;

	/*  CALCULATE FILL SIZE  */
	fillSize_ = BUFFER_SIZE - (2 * padSize_);
}

/******************************************************************************
*
*  function:  initializeFilter
*
*  purpose:   Initializes filter impulse response and impulse delta
*             values.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::initializeFilter()
{
	const TFloat beta = 5.658;           /*  kaiser window parameter  */
	const TFloat lpCutoff = 11.0 / 13.0; /*  (0.846 OF NYQUIST)  */

	/*  INITIALIZE THE FILTER IMPULSE RESPONSE  */
	h_[0] = lpCutoff;
	const TFloat x = M_PI / L_RANGE;
	for (unsigned int i = 1; i < FILTER_LENGTH; i++) {
		const TFloat y = i * x;
		h_[i] = std::sin(y * lpCutoff) / y;
	}

	/*  APPLY A KAISER WINDOW TO THE IMPULSE RESPONSE  */
	const TFloat IBeta = 1.0f / Izero(beta);
	for (unsigned int i = 0; i < FILTER_LENGTH; i++) {
		const TFloat temp = static_cast<TFloat>(i) / FILTER_LENGTH;
		h_[i] *= Izero(beta * std::sqrt(1.0f - (temp * temp))) * IBeta;
	}

	/*  INITIALIZE THE FILTER IMPULSE RESPONSE DELTA VALUES  */
	for (unsigned int i = 0; i < FILTER_LIMIT; i++) {
		deltaH_[i] = h_[i + 1] - h_[i];
	}
	deltaH_[FILTER_LIMIT] = 0.0f - h_[FILTER_LIMIT];
}

/******************************************************************************
*
*  function:  dataFill
*
*  purpose:   Fills the ring buffer with a single sample, increments
*             the counters and pointers, and empties the buffer when
*             full.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::dataFill(TFloat data)
{
	/*  PUT THE DATA INTO THE RING BUFFER  */
	buffer_[fillPtr_] = data;

	/*  INCREMENT THE FILL POINTER, MODULO THE BUFFER SIZE  */
	srIncrement(&fillPtr_, BUFFER_SIZE);

	/*  INCREMENT THE COUNTER, AND EMPTY THE BUFFER IF FULL  */
	if (++fillCounter_ >= fillSize_) {
		dataEmpty();
		/* RESET THE FILL COUNTER  */
		fillCounter_ = 0;
	}
}

/******************************************************************************
*
*  function:  dataEmpty
*
*  purpose:   Converts available portion of the input signal to the
*             new sampling rate, and outputs the samples to the
*             sound struct.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::dataEmpty()
{
	/*  CALCULATE END POINTER  */
	int endPtr = fillPtr_ - padSize_;

	/*  ADJUST THE END POINTER, IF LESS THAN ZERO  */
	if (endPtr < 0) {
		endPtr += BUFFER_SIZE;
	}

	/*  ADJUST THE ENDPOINT, IF LESS THEN THE EMPTY POINTER  */
	if (endPtr < emptyPtr_) {
		endPtr += BUFFER_SIZE;
	}

	/*  UPSAMPLE LOOP (SLIGHTLY MORE EFFICIENT THAN DOWNSAMPLING)  */
	if (sampleRateRatio_ >= 1.0f) {
		while (emptyPtr_ < endPtr) {
			/*  RESET ACCUMULATOR TO ZERO  */
			TFloat output{};

			/*  CALCULATE INTERPOLATION VALUE (STATIC WHEN UPSAMPLING)  */
			TFloat interpolation = static_cast<TFloat>(mValue(timeRegister_)) / M_RANGE;

			/*  COMPUTE THE LEFT SIDE OF THE FILTER CONVOLUTION  */
			int index = emptyPtr_;
			for (unsigned int filterIndex = lValue(timeRegister_);
					filterIndex < FILTER_LENGTH;
					srDecrement(&index,BUFFER_SIZE), filterIndex += filterIncrement_) {
				output += (buffer_[index] *
						(h_[filterIndex] + (deltaH_[filterIndex] * interpolation)));
			}

			/*  ADJUST VALUES FOR RIGHT SIDE CALCULATION  */
			timeRegister_ = ~timeRegister_;
			interpolation = static_cast<TFloat>(mValue(timeRegister_)) / M_RANGE;

			/*  COMPUTE THE RIGHT SIDE OF THE FILTER CONVOLUTION  */
			index = emptyPtr_;
			srIncrement(&index,BUFFER_SIZE);
			for (unsigned int filterIndex = lValue(timeRegister_);
					filterIndex < FILTER_LENGTH;
					srIncrement(&index,BUFFER_SIZE), filterIndex += filterIncrement_) {
				output += (buffer_[index] *
						(h_[filterIndex] + (deltaH_[filterIndex] * interpolation)));
			}

			/*  SAVE THE SAMPLE  */
			output_(static_cast<float>(output));

			/*  CHANGE TIME REGISTER BACK TO ORIGINAL FORM  */
			timeRegister_ = ~timeRegister_;

			/*  INCREMENT THE TIME REGISTER  */
			timeRegister_ += timeRegisterIncrement_;

			/*  INCREMENT THE EMPTY POINTER, ADJUSTING IT AND END POINTER  */
			emptyPtr_ += nValue(timeRegister_);

			if (emptyPtr_ >= static_cast<int>(BUFFER_SIZE)) {
				emptyPtr_ -= BUFFER_SIZE;
				endPtr -= BUFFER_SIZE;
			}

			/*  CLEAR N PART OF TIME REGISTER  */
			timeRegister_ &= (~N_MASK);
		}
	} else {
		/*  DOWNSAMPLING CONVERSION LOOP  */

		while (emptyPtr_ < endPtr) {

			/*  RESET ACCUMULATOR TO ZERO  */
			TFloat output{};

			/*  COMPUTE P PRIME  */
			unsigned int phaseIndex = static_cast<unsigned int>(std::rint(fractionValue(timeRegister_) * sampleRateRatio_));

			/*  COMPUTE THE LEFT SIDE OF THE FILTER CONVOLUTION  */
			int index = emptyPtr_;
			unsigned int impulseIndex;
			while ((impulseIndex = (phaseIndex >> M_BITS)) < FILTER_LENGTH) {
				const TFloat impulse = h_[impulseIndex] + (deltaH_[impulseIndex] *
								(static_cast<TFloat>(mValue(phaseIndex)) / M_RANGE));
				output += buffer_[index] * impulse;
				srDecrement(&index, BUFFER_SIZE);
				phaseIndex += phaseIncrement_;
			}

			/*  COMPUTE P PRIME, ADJUSTED FOR RIGHT SIDE  */
			phaseIndex = static_cast<unsigned int>(std::rint(
						static_cast<TFloat>(fractionValue(~timeRegister_)) * sampleRateRatio_));

			/*  COMPUTE THE RIGHT SIDE OF THE FILTER CONVOLUTION  */
			index = emptyPtr_;
			srIncrement(&index, BUFFER_SIZE);
			while ((impulseIndex = (phaseIndex >> M_BITS)) < FILTER_LENGTH) {
				const TFloat impulse = h_[impulseIndex] + (deltaH_[impulseIndex] *
								(static_cast<TFloat>(mValue(phaseIndex)) / M_RANGE));
				output += buffer_[index] * impulse;
				srIncrement(&index, BUFFER_SIZE);
				phaseIndex += phaseIncrement_;
			}

			/*  SAVE THE SAMPLE  */
			output_(static_cast<float>(output));

			/*  INCREMENT THE TIME REGISTER  */
			timeRegister_ += timeRegisterIncrement_;

			/*  INCREMENT THE EMPTY POINTER, ADJUSTING IT AND END POINTER  */
			emptyPtr_ += nValue(timeRegister_);
			if (emptyPtr_ >= static_cast<int>(BUFFER_SIZE)) {
				emptyPtr_ -= BUFFER_SIZE;
				endPtr -= BUFFER_SIZE;
			}

			/*  CLEAR N PART OF TIME REGISTER  */
			timeRegister_ &= (~N_MASK);
		}
	}
}

/******************************************************************************
*
*  function:  srIncrement
*
*  purpose:   Increments the pointer, keeping it within the range
*             0 to (modulus-1).
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::srIncrement(int *pointer, int modulus)
{
	if (++(*pointer) >= modulus) {
		(*pointer) -= modulus;
	}
}

/******************************************************************************
*
*  function:  srDecrement
*
*  purpose:   Decrements the pointer, keeping it within the range
*             0 to (modulus-1).
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::srDecrement(int *pointer, int modulus)
{
	if (--(*pointer) < 0) {
		(*pointer) += modulus;
	}
}

/******************************************************************************
*
*  function:  flushBuffer
*
*  purpose:   Pads the buffer with zero samples, and flushes it by
*             converting the remaining samples.
*
******************************************************************************/
template<typename TFloat>
void
SampleRateConverter<TFloat>::flushBuffer()
{
	/*  PAD END OF RING BUFFER WITH ZEROS  */
	for (int i = 0; i < padSize_ * 2; i++) {
		dataFill(0.0);
	}

	/*  FLUSH UP TO FILL POINTER - PADSIZE  */
	dataEmpty();
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_SAMPLE_RATE_CONVERTER_H_ */

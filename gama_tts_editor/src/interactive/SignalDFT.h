/***************************************************************************
 *  Copyright 2015 Marcelo Y. Matuda                                       *
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

#ifndef SIGNAL_DFT_H
#define SIGNAL_DFT_H

#include <cmath>

#include "FFTW.h"



namespace GS {

class SignalDFT {
public:
	// size is expected to be a power of two.
	explicit SignalDFT(unsigned int n);
	~SignalDFT();

	// Returns the absolute value of the spectrum.
	// input must point to an array of size n (or bigger).
	// output must point to an array of size n/2 + 1 (or bigger).
	template<typename T, typename U> void execute(const T* input, U* output);

	unsigned int size() const { return n_; }
	unsigned int outputSize() const { return outputN_; }
private:
	SignalDFT(const SignalDFT&) = delete;
	SignalDFT& operator=(const SignalDFT&) = delete;
	SignalDFT(SignalDFT&&) = delete;
	SignalDFT& operator=(SignalDFT&&) = delete;

	unsigned int n_;
	unsigned int outputN_;
	float* in_;
	fftwf_complex* out_;
	FFTWPlan dftPlan_;
};

template<typename T, typename U>
void
SignalDFT::execute(const T* input, U* output)
{
	for (unsigned int i = 0; i < n_; ++i) {
		in_[i] = input[i];
	}
	FFTW::execute(dftPlan_);
	for (unsigned int i = 0; i < outputN_; ++i) {
		const U rVal = out_[i][FFTW::REAL];
		const U iVal = out_[i][FFTW::IMAG];
		output[i] = std::sqrt(rVal * rVal + iVal * iVal);
	}
}

} /* namespace GS */

#endif // SIGNAL_DFT_H

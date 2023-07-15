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

#include "SignalDFT.h"



namespace GS {

SignalDFT::SignalDFT(unsigned int n)
		: n_(n)
		, outputN_(n / 2 + 1)
		, in_(nullptr)
		, out_(nullptr)
{
	in_ = FFTW::alloc_real<float>(n_ * sizeof(float));
	out_ = FFTW::alloc_complex<float>(outputN_ * sizeof(fftwf_complex));
	{
		FFTW fftw;
		dftPlan_ = fftw.plan_dft_r2c_1d(n_, in_, out_, FFTW_ESTIMATE);
	}
}

SignalDFT::~SignalDFT()
{
	{
		FFTW fftw;
		fftw.destroy_plan(dftPlan_);
	}
	FFTW::free(out_);
	FFTW::free(in_);
}

} /* namespace GS */

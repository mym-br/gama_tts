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

#ifndef VTM_RADIATION_FILTER_H_
#define VTM_RADIATION_FILTER_H_



namespace GS {
namespace VTM {

// This is a variable, one-zero, one-pole, highpass filter,
// whose cutoff point is determined by the aperture
// coefficient.
template<typename FloatType>
class RadiationFilter {
public:
	RadiationFilter(FloatType apertureCoeff);
	~RadiationFilter() {}

	void reset();
	FloatType filter(FloatType input);
private:
	RadiationFilter(const RadiationFilter&) = delete;
	RadiationFilter& operator=(const RadiationFilter&) = delete;

	const FloatType a20_;
	const FloatType a21_;
	const FloatType b21_;
	FloatType radiationX_;
	FloatType radiationY_;
};

template<typename FloatType>
RadiationFilter<FloatType>::RadiationFilter(FloatType apertureCoeff)
		: a20_ {apertureCoeff}
		, a21_ {-a20_}
		, b21_ {-a20_}
		, radiationX_ {}
		, radiationY_ {}
{
}

template<typename FloatType>
void
RadiationFilter<FloatType>::reset()
{
	radiationX_ = 0.0;
	radiationY_ = 0.0;
}

template<typename FloatType>
FloatType
RadiationFilter<FloatType>::filter(FloatType input)
{
	const FloatType output = (a20_ * input) + (a21_ * radiationX_) - (b21_ * radiationY_);
	radiationX_ = input;
	radiationY_ = output;
	return output;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_RADIATION_FILTER_H_ */

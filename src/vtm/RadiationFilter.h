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
	FloatType filter(FloatType x);
private:
	RadiationFilter(const RadiationFilter&) = delete;
	RadiationFilter& operator=(const RadiationFilter&) = delete;

	const FloatType b0_;
	const FloatType b1_;
	const FloatType a1_;
	FloatType x1_;
	FloatType y1_;
};

template<typename FloatType>
RadiationFilter<FloatType>::RadiationFilter(FloatType apertureCoeff)
		: b0_ {apertureCoeff}
		, b1_ {-b0_}
		, a1_ {-b0_}
		, x1_ {}
		, y1_ {}
{
}

template<typename FloatType>
void
RadiationFilter<FloatType>::reset()
{
	x1_ = 0.0;
	y1_ = 0.0;
}

template<typename FloatType>
FloatType
RadiationFilter<FloatType>::filter(FloatType x)
{
	const FloatType y = b0_ * x + b1_ * x1_ - a1_ * y1_;
	x1_ = x;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_RADIATION_FILTER_H_ */

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

#ifndef VTM_REFLECTION_FILTER_H_
#define VTM_REFLECTION_FILTER_H_

#include <cmath>



namespace GS {
namespace VTM {

// This is a variable, one-pole lowpass filter, whose cutoff
// is determined by the aperture coefficient.
template<typename FloatType>
class ReflectionFilter {
public:
	ReflectionFilter(FloatType apertureCoeff);
	~ReflectionFilter() {}

	void reset();
	FloatType filter(FloatType input);
private:
	ReflectionFilter(const ReflectionFilter&) = delete;
	ReflectionFilter& operator=(const ReflectionFilter&) = delete;

	const FloatType a10_;
	const FloatType b11_;
	FloatType reflectionY_;
};



template<typename FloatType>
ReflectionFilter<FloatType>::ReflectionFilter(FloatType apertureCoeff)
		: a10_ {1.0f - std::abs(apertureCoeff)}
		, b11_ {-apertureCoeff}
		, reflectionY_ {}
{
}

template<typename FloatType>
void
ReflectionFilter<FloatType>::reset()
{
	reflectionY_ = 0.0;
}

template<typename FloatType>
FloatType
ReflectionFilter<FloatType>::filter(FloatType input)
{
	const FloatType output = (a10_ * input) - (b11_ * reflectionY_);
	reflectionY_ = output;
	return output;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_REFLECTION_FILTER_H_ */

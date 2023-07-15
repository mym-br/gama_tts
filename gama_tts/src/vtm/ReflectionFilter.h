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
template<typename TFloat>
class ReflectionFilter {
public:
	explicit ReflectionFilter(TFloat apertureCoeff);
	~ReflectionFilter() = default;

	void reset();
	TFloat filter(TFloat x);
private:
	ReflectionFilter(const ReflectionFilter&) = delete;
	ReflectionFilter& operator=(const ReflectionFilter&) = delete;
	ReflectionFilter(ReflectionFilter&&) = delete;
	ReflectionFilter& operator=(ReflectionFilter&&) = delete;

	const TFloat b0_;
	const TFloat a1_;
	TFloat y1_;
};



template<typename TFloat>
ReflectionFilter<TFloat>::ReflectionFilter(TFloat apertureCoeff)
		: b0_(1.0f - std::abs(apertureCoeff))
		, a1_(-apertureCoeff)
		, y1_()
{
}

template<typename TFloat>
void
ReflectionFilter<TFloat>::reset()
{
	y1_ = 0.0;
}

template<typename TFloat>
TFloat
ReflectionFilter<TFloat>::filter(TFloat x)
{
	const TFloat y = b0_ * x - a1_ * y1_;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_REFLECTION_FILTER_H_ */

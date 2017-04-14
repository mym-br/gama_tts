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

#ifndef VTM_NOISE_FILTER_H_
#define VTM_NOISE_FILTER_H_



namespace GS {
namespace VTM {

// One-zero lowpass filter.
template<typename FloatType>
class NoiseFilter {
public:
	NoiseFilter();
	~NoiseFilter() {}

	void reset();
	FloatType filter(FloatType x);
private:
	NoiseFilter(const NoiseFilter&) = delete;
	NoiseFilter& operator=(const NoiseFilter&) = delete;

	FloatType x1_;
};



template<typename FloatType>
NoiseFilter<FloatType>::NoiseFilter() : x1_ {}
{
}

template<typename FloatType>
void
NoiseFilter<FloatType>::reset()
{
	x1_ = 0.0;
}

template<typename FloatType>
FloatType
NoiseFilter<FloatType>::filter(FloatType x)
{
	const FloatType y = x + x1_;
	x1_ = x;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_NOISE_FILTER_H_ */

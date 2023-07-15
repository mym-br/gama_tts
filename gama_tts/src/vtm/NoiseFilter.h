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
template<typename TFloat>
class NoiseFilter {
public:
	NoiseFilter();
	~NoiseFilter() = default;

	void reset();
	TFloat filter(TFloat x);
private:
	NoiseFilter(const NoiseFilter&) = delete;
	NoiseFilter& operator=(const NoiseFilter&) = delete;
	NoiseFilter(NoiseFilter&&) = delete;
	NoiseFilter& operator=(NoiseFilter&&) = delete;

	TFloat x1_;
};



template<typename TFloat>
NoiseFilter<TFloat>::NoiseFilter() : x1_()
{
}

template<typename TFloat>
void
NoiseFilter<TFloat>::reset()
{
	x1_ = 0.0;
}

template<typename TFloat>
TFloat
NoiseFilter<TFloat>::filter(TFloat x)
{
	const TFloat y = x + x1_;
	x1_ = x;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_NOISE_FILTER_H_ */

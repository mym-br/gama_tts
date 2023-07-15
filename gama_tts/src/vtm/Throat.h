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

#ifndef VTM_THROAT_H_
#define VTM_THROAT_H_



namespace GS {
namespace VTM {

template<typename TFloat>
class Throat {
public:
	Throat(TFloat sampleRate, TFloat throatCutoff, TFloat throatGain);
	~Throat() = default;

	void reset();
	TFloat process(TFloat x);
private:
	Throat(const Throat&) = delete;
	Throat& operator=(const Throat&) = delete;
	Throat(Throat&&) = delete;
	Throat& operator=(Throat&&) = delete;

	const TFloat b0_;
	const TFloat a1_;
	const TFloat throatGain_;
	TFloat y1_;
};



template<typename TFloat>
Throat<TFloat>::Throat(TFloat sampleRate, TFloat throatCutoff, TFloat throatGain)
		// Initializes the throat lowpass filter coefficients
		// according to the throatCutoff value, and also the
		// throatGain, according to the throatVol value.
		: b0_((throatCutoff * 2.0f) / sampleRate)
		, a1_(b0_ - 1.0f)
		, throatGain_(throatGain)
		, y1_()
{
}

template<typename TFloat>
void
Throat<TFloat>::reset()
{
	y1_ = 0.0;
}

/******************************************************************************
*
*  function:  throat
*
*  purpose:   Simulates the radiation of sound through the walls
*             of the throat.
*
******************************************************************************/
template<typename TFloat>
TFloat
Throat<TFloat>::process(TFloat x)
{
	const TFloat y = b0_ * x - a1_ * y1_;
	y1_ = y;
	return y * throatGain_;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_THROAT_H_ */

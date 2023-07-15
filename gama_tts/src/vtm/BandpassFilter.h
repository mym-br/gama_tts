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

#ifndef VTM_BANDPASS_FILTER_H_
#define VTM_BANDPASS_FILTER_H_

#include <cmath>



namespace GS {
namespace VTM {

// Bandpass filter, with variable center frequency and bandwidth.
template<typename TFloat>
class BandpassFilter {
public:
	BandpassFilter();
	~BandpassFilter() = default;

	void reset();
	void update(TFloat sampleRate, TFloat bandwidth, TFloat centerFreq);
	TFloat filter(TFloat x);
private:
	BandpassFilter(const BandpassFilter&) = delete;
	BandpassFilter& operator=(const BandpassFilter&) = delete;
	BandpassFilter(BandpassFilter&&) = delete;
	BandpassFilter& operator=(BandpassFilter&&) = delete;

	TFloat b0_;
	TFloat a2_;
	TFloat a1_;
	TFloat x1_;
	TFloat x2_;
	TFloat y1_;
	TFloat y2_;
	TFloat prevSampleRate_;
	TFloat prevBandwidth_;
	TFloat prevCenterFreq_;
};



template<typename TFloat>
BandpassFilter<TFloat>::BandpassFilter()
		: b0_()
		, a2_()
		, a1_()
		, x1_()
		, x2_()
		, y1_()
		, y2_()
		, prevSampleRate_(-1.0)
		, prevBandwidth_(-1.0)
		, prevCenterFreq_(-1.0)
{
}

template<typename TFloat>
void
BandpassFilter<TFloat>::reset()
{
	x1_ = 0.0;
	x2_ = 0.0;
	y1_ = 0.0;
	y2_ = 0.0;
	prevSampleRate_ = -1.0;
	prevBandwidth_  = -1.0;
	prevCenterFreq_ = -1.0;
}

template<typename TFloat>
void
BandpassFilter<TFloat>::update(TFloat sampleRate, TFloat bandwidth, TFloat centerFreq)
{
	if (sampleRate == prevSampleRate_ && bandwidth == prevBandwidth_ && centerFreq == prevCenterFreq_) {
		return;
	} else {
		prevSampleRate_ = sampleRate;
		prevBandwidth_  = bandwidth;
		prevCenterFreq_ = centerFreq;
	}

	constexpr TFloat pi = M_PI;
	const TFloat T = 1.0f / sampleRate;
	const TFloat tanValue = std::tan(pi * bandwidth * T);
	const TFloat cosValue = std::cos(2.0f * pi * centerFreq * T);
	a2_ = (1.0f - tanValue) / (1.0f + tanValue);
	a1_ = -(1.0f + a2_) * cosValue;
	b0_ = 0.5f - 0.5f * a2_;
	// b1_ = 0.0
	// b2_ = -b0_
}

template<typename TFloat>
TFloat
BandpassFilter<TFloat>::filter(TFloat x)
{
	const TFloat y = b0_ * (x - x2_) - a1_ * y1_ - a2_ * y2_;
	x2_ = x1_;
	x1_ = x;
	y2_ = y1_;
	y1_ = y;
	return y;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_BANDPASS_FILTER_H_ */

/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#ifndef VTM_ROSENBERG_B_GLOTTAL_SOURCE_H_
#define VTM_ROSENBERG_B_GLOTTAL_SOURCE_H_

#include <cmath> /* sin */

#include "Exception.h"



namespace GS {
namespace VTM {

// Rosenberg, A. E. Effect of glottal pulse shape on the quality of natural vowels.
// The Journal of the Acoustical Society of America, 1970, 49-2, 583-590.
template<typename TFloat>
class RosenbergBGlottalSource {
public:
	enum class Type {
		pulse,
		sine
	};

	RosenbergBGlottalSource(
			Type type, TFloat sampleRate,
			TFloat tp, TFloat tnMin, TFloat tnMax);
	~RosenbergBGlottalSource() = default;

	void reset();
	TFloat getSample(TFloat frequency /* Hz */);
	void setup(TFloat amplitude /* [0.0, 1.0] */);
private:
	RosenbergBGlottalSource(const RosenbergBGlottalSource&) = delete;
	RosenbergBGlottalSource& operator=(const RosenbergBGlottalSource&) = delete;
	RosenbergBGlottalSource(RosenbergBGlottalSource&&) = delete;
	RosenbergBGlottalSource& operator=(RosenbergBGlottalSource&&) = delete;

	const Type type_;
	const TFloat sampleRate_;
	const TFloat tnMin_;
	const TFloat tnMax_;
	TFloat t1_; // time of the peak of the pulse
	TFloat t2_; // time of the end of the pulse
	TFloat nextT2_; // t2_ will have this value at the next cycle
	TFloat prevAmplitude_;
	TFloat t_; // 0 to 1
};



template<typename TFloat>
RosenbergBGlottalSource<TFloat>::RosenbergBGlottalSource(
			Type type, TFloat sampleRate,
			TFloat tp, TFloat tnMin, TFloat tnMax)
		: type_(type)
		, sampleRate_(sampleRate)
		, tnMin_(tnMin / 100.0f)
		, tnMax_(tnMax / 100.0f)
		, t1_(tp / 100.0f)
		, t2_(t1_ + tnMax_)
		, nextT2_(t2_)
		, prevAmplitude_(-1.0)
		, t_()
{
	constexpr TFloat eps = 1.0e-2;

	if (t1_ < eps) {
		THROW_EXCEPTION(InvalidParameterException, "Glottal source: tp too small or negative.");
	}
	if (tnMin_ < eps) {
		THROW_EXCEPTION(InvalidParameterException, "Glottal source: tnMin too small or negative.");
	}
	if (tnMax_ < eps) {
		THROW_EXCEPTION(InvalidParameterException, "Glottal source: tnMax too small or negative.");
	}
	if (tnMin_ > tnMax_) {
		THROW_EXCEPTION(InvalidParameterException, "Glottal source: tnMin must be <= tnMax.");
	}
	if (t1_ + tnMax_ > 1.0f) {
		THROW_EXCEPTION(InvalidParameterException, "Glottal source: tp + tnMax must be <= 1.");
	}
}

template<typename TFloat>
void
RosenbergBGlottalSource<TFloat>::reset()
{
	t2_ = t1_ + tnMax_;
	nextT2_ = t2_;
	prevAmplitude_ = -1.0;
	t_ = 0.0;
}

template<typename TFloat>
void
RosenbergBGlottalSource<TFloat>::setup(TFloat amplitude)
{
	if (tnMin_ == tnMax_ || amplitude == prevAmplitude_) {
		return;
	}

	nextT2_ = t1_ + tnMax_ - amplitude * (tnMax_ - tnMin_);
	prevAmplitude_ = amplitude;
}

template<typename TFloat>
TFloat
RosenbergBGlottalSource<TFloat>::getSample(TFloat frequency)
{
	TFloat value;

	if (type_ == Type::pulse) {
		if (t_ < t1_) {
			const TFloat x = t_ / t1_;
			value = (x * x) * (3.0f - 2.0f * x);
		} else if (t_ < t2_) {
			const TFloat x = (t_ - t1_) / (t2_ - t1_);
			value = 1.0f - x * x;
		} else {
			value = 0.0;
		}
	} else {
		value = std::sin(t_ * TFloat{2.0 * M_PI});
	}

	const TFloat dt = frequency / sampleRate_;
	t_ += dt;
	if (t_ > 1.0f) {
		t_ -= 1.0f;
		t2_ = nextT2_;
	}

	return value;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* VTM_ROSENBERG_B_GLOTTAL_SOURCE_H_ */

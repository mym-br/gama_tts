/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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

#ifndef MOVING_AVERAGE_FILTER_H_
#define MOVING_AVERAGE_FILTER_H_

#include <cassert>
#include <cstddef> /* std::size_t */
#include <cmath> /* round */
#include <vector>



namespace GS {
namespace VTM {

/*******************************************************************************
 * Moving average filter.
 */
template<typename TFloat>
class MovingAverageFilter {
public:
	MovingAverageFilter(TFloat sampleRate, TFloat period /* seconds */);

	void reset();
	TFloat filter(TFloat value);
private:
	std::vector<TFloat> buf_;
	typename std::vector<TFloat>::size_type pos_;
	double sum_;
	double invN_;
};



/*******************************************************************************
 * Constructor.
 */
template<typename TFloat>
MovingAverageFilter<TFloat>::MovingAverageFilter(TFloat sampleRate, TFloat period)
		: buf_(static_cast<std::size_t>(std::round(sampleRate * period)))
		, pos_(buf_.size())
		, sum_()
		, invN_(1.0 / buf_.size())
{
	assert(!buf_.empty());
}

/*******************************************************************************
 *
 */
template<typename TFloat>
void
MovingAverageFilter<TFloat>::reset()
{
	for (auto& item : buf_) item = 0.0;
	pos_ = buf_.size();
	sum_ = 0.0;
}

/*******************************************************************************
 *
 */
template<typename TFloat>
TFloat
MovingAverageFilter<TFloat>::filter(TFloat value) {
	if (++pos_ >= buf_.size()) {
		if (pos_ > buf_.size()) { // first value
			buf_.assign(buf_.size(), value);
			sum_ = value * static_cast<double>(buf_.size());
		}
		pos_ = 0;
	}
	sum_ -= buf_[pos_];
	sum_ += value;
	buf_[pos_] = value;
	return sum_ * invN_;
}

} /* namespace VTM */
} /* namespace GS */

#endif /* MOVING_AVERAGE_FILTER_H_ */

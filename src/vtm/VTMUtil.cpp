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

#include "VTMUtil.h"

#define MIN_MAXIMUM_ABS_SAMPLE_VALUE (1.0e-30)
#define MAX_OUTPUT_ABS_SAMPLE_VALUE (0.95)



namespace GS {
namespace VTM {
namespace Util {

std::size_t
getSamples(std::vector<float>& inBuffer, std::size_t& inBufferPos, float* outBuffer, std::size_t n)
{
	if (inBuffer.empty()) return 0;

	const std::size_t size = inBuffer.size();
	const std::size_t initialPos = inBufferPos;
	for (std::size_t i = 0; i < n && inBufferPos < size; ++i, ++inBufferPos) {
		outBuffer[i] = inBuffer[inBufferPos];
	}

	const std::size_t samplesRead = inBufferPos - initialPos;
	if (inBufferPos == size) { // all samples were used
		inBuffer.clear();
		inBufferPos = 0;
	}
	return samplesRead;
}

float
calculateOutputScale(const std::vector<float>& buffer)
{
	const float maxValue = maximumAbsoluteValue(buffer);
	if (maxValue < float{MIN_MAXIMUM_ABS_SAMPLE_VALUE}) {
		return 0.0;
	}

	return MAX_OUTPUT_ABS_SAMPLE_VALUE / maxValue;
}

} /* namespace Util */
} /* namespace VTM */
} /* namespace GS */

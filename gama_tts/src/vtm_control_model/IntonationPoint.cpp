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

#include "IntonationPoint.h"

#include "EventList.h"
#include "Exception.h"



namespace GS {
namespace VTMControlModel {

IntonationPoint::IntonationPoint(const EventList* eventList)
		: semitone_()
		, offsetTime_()
		, slope_()
		, ruleIndex_()
		, eventList_(eventList)
{
	if (eventList_ == nullptr) {
		THROW_EXCEPTION(InvalidParameterException, "The pointer to event list is null.");
	}
}

double
IntonationPoint::absoluteTime() const
{
	double time = eventList_->getBeatAtIndex(ruleIndex_) + offsetTime_;
	if (time < 0.0) {
		return 0.0;
	} else {
		return time;
	}
}

double
IntonationPoint::beatTime() const
{
	return eventList_->getBeatAtIndex(ruleIndex_);
}

} /* namespace VTMControlModel */
} /* namespace GS */

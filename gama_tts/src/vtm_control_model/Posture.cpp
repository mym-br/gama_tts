/***************************************************************************
 *  Copyright 2014 Marcelo Y. Matuda                                       *
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
// This file was created by Marcelo Y. Matuda, and code/information
// from Gnuspeech was added to it later.

#include "Posture.h"



namespace GS {
namespace VTMControlModel {

/*******************************************************************************
 * Constructor.
 */
Posture::Posture(const std::string& name, unsigned int numParameters, unsigned int numSymbols)
		: name_(name)
		, parameterTargetList_(numParameters)
		, symbolTargetList_(numSymbols)
{
	if (numParameters == 0) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid number of parameters: 0.");
	}
	if (numSymbols != NUM_SYMBOLS) {
		THROW_EXCEPTION(InvalidParameterException, "Invalid number of symbols: "
					<< numSymbols << "(should be " << NUM_SYMBOLS << ").");
	}

	auto newCategory = std::make_shared<Category>(name);
	newCategory->setNative();
	categoryList_.push_back(newCategory);
}

/*******************************************************************************
 *
 */
const std::shared_ptr<Category>
Posture::findCategory(const std::string& name) const
{
	for (auto& category : categoryList_) {
		if (category->name() == name) {
			return category;
		}
	}
	return std::shared_ptr<Category>();
}

/*******************************************************************************
 *
 */
bool
Posture::isMemberOfCategory(const Category& category) const
{
	for (const auto& postureCat : categoryList_) {
		if (postureCat.get() == &category) {
			return true;
		}
	}
	return false;
}

/*******************************************************************************
 *
 */
std::unique_ptr<Posture>
Posture::copy(const std::string& newName) const
{
	auto newPosture = std::make_unique<Posture>(newName, parameterTargetList_.size(), symbolTargetList_.size());

	for (const auto& category : categoryList_) {
		if (!category->native()) {
			newPosture->categoryList_.push_back(category);
		}
	}
	newPosture->parameterTargetList_ = parameterTargetList_;
	newPosture->symbolTargetList_ = symbolTargetList_;
	newPosture->comment_ = comment_;

	return newPosture;
}

} /* namespace VTMControlModel */
} /* namespace GS */

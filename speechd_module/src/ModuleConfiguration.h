/***************************************************************************
 *  Copyright 2015 Marcelo Y. Matuda                                       *
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

#ifndef MODULE_CONFIGURATION_H_
#define MODULE_CONFIGURATION_H_

#include <string>



struct ModuleConfiguration {
	enum class PunctuationMode {
		none,
		some,
		all
	};
	enum class CapitalLetterRecognition {
		none,
		spell,
		icon
	};

	unsigned int logLevel;
	float pitch;
	float rate;
	float volume;
	PunctuationMode punctuationMode;
	bool spellingMode;
	CapitalLetterRecognition capitalLetterRecognition;
	std::string voice;
	std::string language;
	std::string synthesisVoice;

	ModuleConfiguration();

	void setLogLevel(const std::string& s);
	void setPitch(const std::string& s);
	void setRate(const std::string& s);
	void setVolume(const std::string& s);
	void setPunctuationMode(const std::string& s);
	void setSpellingMode(const std::string& s);
	void setCapitalLetterRecognition(const std::string& s);
	void setVoice(const std::string& s);
	void setLanguage(const std::string& s);
	void setSynthesisVoice(const std::string& s);
};

#endif /* MODULE_CONFIGURATION_H_ */

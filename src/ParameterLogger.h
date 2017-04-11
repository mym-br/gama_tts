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

#ifndef GS_PARAMETER_LOGGER_H_
#define GS_PARAMETER_LOGGER_H_

#include <fstream>
#include <iostream>
#include <string>
#include <utility> /* pair */
#include <vector>

#define GS_LOG_PARAMETER(L,P,V) do { L.put(P, V, #P); } while (false)



namespace GS {

template<typename T>
class ParameterLogger {
public:
	ParameterLogger();
	~ParameterLogger();

	void put(unsigned int param, T value, const char* fileName);
private:
	ParameterLogger(const ParameterLogger&) = delete;
	ParameterLogger& operator=(const ParameterLogger&) = delete;

	std::vector<std::pair<std::string, std::vector<T>>> data_;
};

template<typename T>
ParameterLogger<T>::ParameterLogger()
{
}

template<typename T>
ParameterLogger<T>::~ParameterLogger()
{
	int i = -1;
	for (const auto& param : data_) {
		++i;
		if (param.first.empty()) {
			std::cerr << "[GS::ParameterLogger] Empty file name (parameter " << i << ")." << std::endl;
			continue;
		}
		std::ofstream out(param.first);
		if (!out) {
			std::cerr << "[GS::ParameterLogger] Could not open the file " << param.first << '.' << std::endl;
			continue;
		}
		for (const T value : param.second) {
			out << value << '\n';
		}
	}
}

template<typename T>
void
ParameterLogger<T>::put(unsigned int param, T value, const char* fileName)
{
	if (data_.size() <= param) {
		data_.resize(param + 1);
	}
	if (data_[param].first.empty()) { // empty file name
		data_[param].first = std::string(fileName) + ".txt";
	}
	data_[param].second.push_back(value);
}

} /* namespace GS */

#endif /* GS_PARAMETER_LOGGER_H_ */

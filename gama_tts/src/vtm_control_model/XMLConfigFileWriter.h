/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
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

#ifndef VTM_CONTROL_MODEL_XML_CONFIG_FILE_WRITER_H_
#define VTM_CONTROL_MODEL_XML_CONFIG_FILE_WRITER_H_

#include <string>



namespace GS {

class StreamXMLWriter;

namespace VTMControlModel {

class Model;

/*******************************************************************************
 *
 */
class XMLConfigFileWriter {
public:
	XMLConfigFileWriter(const Model& model, const std::string& filePath);
	~XMLConfigFileWriter() = default;

	void saveModel();
private:
	XMLConfigFileWriter(const XMLConfigFileWriter&) = delete;
	XMLConfigFileWriter& operator=(const XMLConfigFileWriter&) = delete;
	XMLConfigFileWriter(XMLConfigFileWriter&&) = delete;
	XMLConfigFileWriter& operator=(XMLConfigFileWriter&&) = delete;

	void writeElements(StreamXMLWriter& xml);

	const Model& model_;
	std::string filePath_;
};

} /* namespace VTMControlModel */
} /* namespace GS */

#endif /* VTM_CONTROL_MODEL_XML_CONFIG_FILE_WRITER_H_ */

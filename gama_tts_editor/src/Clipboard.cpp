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

#include "Clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QTextStream>

#define POSTURE_PARAMETERS_HEADER "# gama_tts posture parameters\n"



namespace GS {
namespace Clipboard {

void
putPostureParameters(const QHash<QString, float>& paramMap)
{
	QString text(POSTURE_PARAMETERS_HEADER);
	for (auto iter = paramMap.constBegin(); iter != paramMap.constEnd(); ++iter) {
		QString key = iter.key();
		text += QString("%1 = %2\n").arg(key.replace(' ', '_')).arg(iter.value());
	}

	QClipboard* clipboard = QApplication::clipboard();
	clipboard->setText(text);
}

QHash<QString, float>
getPostureParameters()
{
	QHash<QString, float> paramMap;
	QClipboard* clipboard = QApplication::clipboard();
	QString text = clipboard->text();
	if (text.isEmpty()) return paramMap;

	if (!text.startsWith(POSTURE_PARAMETERS_HEADER)) {
		qDebug() << "Wrong clipboard data.";
		return paramMap;
	}

	QTextStream in(&text);
	QString line;
	in.readLineInto(&line); // ignore header
	while (in.readLineInto(&line)) {
		QTextStream lineStream(&line);
		QString name;
		QString separator;
		float value;
		lineStream >> name >> separator >> value;
		if (separator != "=") {
			qDebug() << "Wrong clipboard separator.";
			return paramMap;
		}

		paramMap[name] = value;
	}

	return paramMap;
}

} // namespace Clipboard
} // namespace GS

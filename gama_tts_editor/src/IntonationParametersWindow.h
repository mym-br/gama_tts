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

#ifndef INTONATION_PARAMETERS_WINDOW_H
#define INTONATION_PARAMETERS_WINDOW_H

#include <memory>

#include <QWidget>



namespace Ui {
class IntonationParametersWindow;
}

namespace GS {

struct Synthesis;

class IntonationParametersWindow : public QWidget {
	Q_OBJECT
public:
	explicit IntonationParametersWindow(QWidget* parent=nullptr);
	virtual ~IntonationParametersWindow();

	void clear();
	void setup(Synthesis* synthesis);
private slots:
	void on_updateButton_clicked();
private:
	IntonationParametersWindow(const IntonationParametersWindow&) = delete;
	IntonationParametersWindow& operator=(const IntonationParametersWindow&) = delete;
	IntonationParametersWindow(IntonationParametersWindow&&) = delete;
	IntonationParametersWindow& operator=(IntonationParametersWindow&&) = delete;

	std::unique_ptr<Ui::IntonationParametersWindow> ui_;
	Synthesis* synthesis_;
};

} // namespace GS

#endif // INTONATION_PARAMETERS_WINDOW_H

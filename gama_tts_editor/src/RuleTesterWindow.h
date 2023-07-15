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

#ifndef RULE_TESTER_WINDOW_H
#define RULE_TESTER_WINDOW_H

#include <memory>

#include <QWidget>



namespace Ui {
class RuleTesterWindow;
}

namespace GS {

namespace VTMControlModel {
class Model;
}

class RuleTesterWindow : public QWidget {
	Q_OBJECT
public:
	explicit RuleTesterWindow(QWidget* parent=nullptr);
	virtual ~RuleTesterWindow();

	void resetModel(VTMControlModel::Model* model);
private slots:
	void on_shiftButton_clicked();
	void on_testButton_clicked();
private:
	RuleTesterWindow(const RuleTesterWindow&) = delete;
	RuleTesterWindow& operator=(const RuleTesterWindow&) = delete;
	RuleTesterWindow(RuleTesterWindow&&) = delete;
	RuleTesterWindow& operator=(RuleTesterWindow&&) = delete;

	void clearResults();

	std::unique_ptr<Ui::RuleTesterWindow> ui_;

	VTMControlModel::Model* model_;
};

} // namespace GS

#endif // RULE_TESTER_WINDOW_H

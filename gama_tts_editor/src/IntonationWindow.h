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

#ifndef INTONATION_WINDOW_H
#define INTONATION_WINDOW_H

#include <memory>

#include <QWidget>

namespace Ui {
class IntonationWindow;
}

namespace GS {

struct Synthesis;

class IntonationWindow : public QWidget {
	Q_OBJECT
public:
	explicit IntonationWindow(QWidget* parent=nullptr);
	virtual ~IntonationWindow();

	void clear();
	void setup(Synthesis* synthesis);
signals:
	void synthesisRequested();
	void synthesisToFileRequested(QString filePath);
public slots:
	void on_synthesizeButton_clicked();
	void on_synthesizeToFileButton_clicked();
	void loadIntonationFromEventList();
	void enableProcessingButtons();
	void disableProcessingButtons();
private slots:
	void on_valueLineEdit_editingFinished();
	void on_slopeLineEdit_editingFinished();
	void on_beatOffsetLineEdit_editingFinished();
	void setPointData(
		double value,
		double slope,
		double beat,
		double beatOffset,
		double absoluteTime);
private:
	IntonationWindow(const IntonationWindow&) = delete;
	IntonationWindow& operator=(const IntonationWindow&) = delete;
	IntonationWindow(IntonationWindow&&) = delete;
	IntonationWindow& operator=(IntonationWindow&&) = delete;

	std::unique_ptr<Ui::IntonationWindow> ui_;
	Synthesis* synthesis_;
};

} // namespace GS

#endif // INTONATION_WINDOW_H

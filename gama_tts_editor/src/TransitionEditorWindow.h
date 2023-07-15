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

#ifndef TRANSITION_EDITOR_WINDOW_H
#define TRANSITION_EDITOR_WINDOW_H

#include <memory>

#include <QWidget>

#include "TransitionPoint.h"



class QTableWidgetItem;
class QTreeWidgetItem;

namespace Ui {
class TransitionEditorWindow;
}

namespace GS {

namespace VTMControlModel {
class Model;
class Transition;
}

class TransitionEditorWindow : public QWidget {
	Q_OBJECT
public:
	explicit TransitionEditorWindow(QWidget* parent=nullptr);
	virtual ~TransitionEditorWindow();

	void setSpecial();
	void resetModel(VTMControlModel::Model* model);
signals:
	void equationReferenceChanged();
	void transitionChanged();
public slots:
	void clear();
	void updateEquationsTree();
	void handleEditTransitionButtonClicked(unsigned int transitionGroupIndex, unsigned int transitionIndex);
	void updateTransition();
private slots:
	void on_equationsTree_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
	void on_equationsTree_itemClicked(QTreeWidgetItem* item, int column);
	void on_pointsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void on_pointsTable_itemChanged(QTableWidgetItem* item);
	void on_removePointButton_clicked();
	void on_updateTransitionButton_clicked();
	void on_transitionTypeComboBox_currentIndexChanged(int index);
	void createPoint(unsigned int pointType, float time, float value);
	void handlePointSelected(unsigned int pointIndex);
protected:
	virtual void closeEvent(QCloseEvent* event);
private:
	TransitionEditorWindow(const TransitionEditorWindow&) = delete;
	TransitionEditorWindow& operator=(const TransitionEditorWindow&) = delete;
	TransitionEditorWindow(TransitionEditorWindow&&) = delete;
	TransitionEditorWindow& operator=(TransitionEditorWindow&&) = delete;

	void fillDefaultParameters();
	void updateTransitionWidget();
	void updatePointsTable();
	int getPointListIndexFromTableRow(int row);

	std::unique_ptr<Ui::TransitionEditorWindow> ui_;
	bool special_;
	VTMControlModel::Model* model_;
	VTMControlModel::Transition* transition_;
	VTMControlModel::Transition::Type transitionType_;
	std::vector<TransitionPoint> pointList_;
	float ruleDuration_;
	float ruleMark1_;
	float ruleMark2_;
	float ruleMark3_;
};

} // namespace GS

#endif // TRANSITION_EDITOR_WINDOW_H

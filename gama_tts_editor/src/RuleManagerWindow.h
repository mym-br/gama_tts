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

#ifndef RULE_MANAGER_WINDOW_H
#define RULE_MANAGER_WINDOW_H

#include <memory>

#include <QWidget>



class QString;
class QTableWidgetItem;
class QTreeWidgetItem;

namespace Ui {
class RuleManagerWindow;
}

namespace GS {

namespace VTMControlModel {
class Model;
class Rule;
}

class RuleManagerWindow : public QWidget {
	Q_OBJECT
public:
	explicit RuleManagerWindow(QWidget* parent=nullptr);
	virtual ~RuleManagerWindow();

	void resetModel(VTMControlModel::Model* model);
signals:
	void categoryReferenceChanged();
	void transitionReferenceChanged();
	void specialTransitionReferenceChanged();
	void equationReferenceChanged();
public slots:
	void unselectRule();
	void loadRuleData();
	void setupRuleTransitionsTable();
	void setupTransitionsTree();
	void setupRuleSpecialTransitionsTable();
	void setupSpecialTransitionsTree();
	void setupRuleSymbolEquationsTable();
	void setupEquationsTree();
private slots:
	void on_removeButton_clicked();
	void on_addButton_clicked();
	void on_updateButton_clicked();
	void on_moveUpButton_clicked();
	void on_moveDownButton_clicked();
	void on_rulesTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void on_clearSpecialTransitionButton_clicked();
	void on_clearEquationButton_clicked();
	void on_ruleTransitionsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void on_ruleSpecialTransitionsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void on_ruleSymbolEquationsTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void on_transitionsTree_itemClicked(QTreeWidgetItem* item, int column);
	void on_specialTransitionsTree_itemClicked(QTreeWidgetItem* item, int column);
	void on_equationsTree_itemClicked(QTreeWidgetItem* item, int column);
	void on_updateCommentButton_clicked();
private:
	RuleManagerWindow(const RuleManagerWindow&) = delete;
	RuleManagerWindow& operator=(const RuleManagerWindow&) = delete;
	RuleManagerWindow(RuleManagerWindow&&) = delete;
	RuleManagerWindow& operator=(RuleManagerWindow&&) = delete;

	void setupRulesList();
	void clearRuleData();
	void showRuleStatistics(const VTMControlModel::Rule& rule);
	unsigned int numInputExpressions(const QString& exp1, const QString& exp2, const QString& exp3, const QString& exp4);

	std::unique_ptr<Ui::RuleManagerWindow> ui_;

	VTMControlModel::Model* model_;
	VTMControlModel::Rule* selectedRule_;
};

} // namespace GS

#endif // RULE_MANAGER_WINDOW_H

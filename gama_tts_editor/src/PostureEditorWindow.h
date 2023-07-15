/***************************************************************************
 *  Copyright 2014, 2015, 2017 Marcelo Y. Matuda                           *
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

#ifndef POSTURE_EDITOR_WINDOW_H
#define POSTURE_EDITOR_WINDOW_H

#include <memory>

#include <QRegularExpression>
#include <QWidget>



class QString;
class QTableWidgetItem;
template<typename T, typename U> class QHash;

namespace Ui {
class PostureEditorWindow;
}

namespace GS {

namespace VTMControlModel {
class Model;
class Posture;
}

class PostureEditorWindow : public QWidget {
	Q_OBJECT
public:
	explicit PostureEditorWindow(QWidget* parent=nullptr);
	virtual ~PostureEditorWindow();

	void resetModel(VTMControlModel::Model* model);
signals:
	void postureChanged();
	void postureCategoryChanged();
	void currentPostureChanged(const QHash<QString, float>& paramMap);
public slots:
	void unselectPosture();
private slots:
	void on_addPostureButton_clicked();
	void on_removePostureButton_clicked();
	void on_updatePostureCommentButton_clicked();
	void on_posturesTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem*);
	void on_posturesTable_itemChanged(QTableWidgetItem* item);
	void on_categoriesTable_itemChanged(QTableWidgetItem* item);
	void on_parametersTable_itemChanged(QTableWidgetItem* item);
	void on_useDefaultParameterValueButton_clicked();
	void on_symbolsTable_itemChanged(QTableWidgetItem* item);
	void on_useDefaultSymbolValueButton_clicked();
	void on_copyParametersButton_clicked();
	void on_pasteParametersButton_clicked();
private:
	PostureEditorWindow(const PostureEditorWindow&) = delete;
	PostureEditorWindow& operator=(const PostureEditorWindow&) = delete;
	PostureEditorWindow(PostureEditorWindow&&) = delete;
	PostureEditorWindow& operator=(PostureEditorWindow&&) = delete;

	void setupPosturesTable();
	void clearPostureData();
	void setupCategoriesTable(const VTMControlModel::Posture& posture);
	void setupParametersTable(const VTMControlModel::Posture& posture);
	void setupSymbolsTable(const VTMControlModel::Posture& posture);
	void fillParametersMap(QHash<QString, float>& paramMap);

	std::unique_ptr<Ui::PostureEditorWindow> ui_;
	VTMControlModel::Model* model_;
	QRegularExpression postureNameRegExp_;
};

} // namespace GS

#endif // POSTURE_EDITOR_WINDOW_H

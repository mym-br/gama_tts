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

#ifndef DATA_ENTRY_WINDOW_H
#define DATA_ENTRY_WINDOW_H

#include <memory>

#include <QModelIndex>
#include <QWidget>



namespace Ui {
class DataEntryWindow;
}

namespace GS {

class CategoryModel;
class ParameterModel;
class SymbolModel;
namespace VTMControlModel {
class Model;
}

class DataEntryWindow : public QWidget {
	Q_OBJECT
public:
	explicit DataEntryWindow(QWidget* parent=nullptr);
	virtual ~DataEntryWindow();

	void resetModel(VTMControlModel::Model* model);
signals:
	void categoryChanged();
	void parameterChanged();
	void symbolChanged();
public slots:
	void updateCategoriesTable();
private slots:
	void on_moveCategoryDownButton_clicked();
	void on_moveCategoryUpButton_clicked();
	void on_addCategoryButton_clicked();
	void on_removeCategoryButton_clicked();
	void showCategoryData(const QModelIndex& current, const QModelIndex& previous);
	void on_updateCategoryCommentButton_clicked();

	void on_moveParameterDownButton_clicked();
	void on_moveParameterUpButton_clicked();
	void on_addParameterButton_clicked();
	void on_removeParameterButton_clicked();
	void showParameterData(const QModelIndex& current, const QModelIndex& previous);
	void on_updateParameterCommentButton_clicked();

	void on_moveSymbolDownButton_clicked();
	void on_moveSymbolUpButton_clicked();
	void on_addSymbolButton_clicked();
	void on_removeSymbolButton_clicked();
	void showSymbolData(const QModelIndex& current, const QModelIndex& previous);
	void on_updateSymbolCommentButton_clicked();

	void showError(QString msg);
private:
	DataEntryWindow(const DataEntryWindow&) = delete;
	DataEntryWindow& operator=(const DataEntryWindow&) = delete;
	DataEntryWindow(DataEntryWindow&&) = delete;
	DataEntryWindow& operator=(DataEntryWindow&&) = delete;

	std::unique_ptr<Ui::DataEntryWindow> ui_;
	CategoryModel* categoryModel_;
	ParameterModel* parameterModel_;
	SymbolModel* symbolModel_;
};

} // namespace GS

#endif // DATA_ENTRY_WINDOW_H

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

#ifndef CATEGORY_MODEL_H
#define CATEGORY_MODEL_H

#include <QAbstractTableModel>
#include <QString>



namespace GS {

namespace VTMControlModel {
class Model;
}

class CategoryModel : public QAbstractTableModel {
	Q_OBJECT
public:
	enum {
		NUM_COLUMNS = 2
	};

	explicit CategoryModel(QObject* parent=nullptr);
	virtual ~CategoryModel() = default;

	virtual int rowCount(const QModelIndex& parent=QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent=QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;

	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role=Qt::EditRole);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role=Qt::DisplayRole) const;
	virtual bool insertRows(int row, int count, const QModelIndex& parent=QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex& parent=QModelIndex());

	void resetModel(VTMControlModel::Model* model);
	QString getCategoryComment(const QModelIndex& index) const;
	void setCategoryComment(const QModelIndex& index, const QString& comment) const;
	QModelIndex incrementCategoryRow(const QModelIndex& index);
	QModelIndex decrementCategoryRow(const QModelIndex& index);
signals:
	void categoryChanged();
	void errorOccurred(QString msg);
public slots:
	void updateView();
private:
	CategoryModel(const CategoryModel&) = delete;
	CategoryModel& operator=(const CategoryModel&) = delete;
	CategoryModel(CategoryModel&&) = delete;
	CategoryModel& operator=(CategoryModel&&) = delete;

	VTMControlModel::Model* model_;
};

} // namespace GS

#endif // CATEGORY_MODEL_H

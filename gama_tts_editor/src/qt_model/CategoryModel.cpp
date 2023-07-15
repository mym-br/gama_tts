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

#include "CategoryModel.h"

#include <cstddef> /* std::size_t */
#include <memory>
#include <utility> /* swap */

#include "Model.h"

#define NEW_ITEM_NAME "___new___"



namespace GS {

CategoryModel::CategoryModel(QObject* parent)
		: QAbstractTableModel(parent)
		, model_()
{
}



int
CategoryModel::rowCount(const QModelIndex& /*parent*/) const
{
	return model_ == nullptr ? 0 : model_->categoryList().size();
}

int
CategoryModel::columnCount(const QModelIndex& /*parent*/) const
{
	return NUM_COLUMNS;
}

QVariant
CategoryModel::data(const QModelIndex& index, int role) const
{
	if (model_ == nullptr || !index.isValid()) {
		return QVariant();
	}
	unsigned int row = index.row();
	if (row >= model_->categoryList().size()) {
		return QVariant();
	}

	switch (role) {
	case Qt::DisplayRole:
	case Qt::EditRole:
		if (index.column() == 0) {
			return static_cast<int>(model_->categoryList()[row].use_count()) - 1;
		} else if (index.column() == 1) {
			return model_->categoryList()[row]->name().c_str();
		}
		break;
	case Qt::CheckStateRole:
		if (index.column() == 0) {
			return model_->categoryList()[row].use_count() > 1 ? Qt::Checked : Qt::Unchecked;
		}
		break;
	}

	return QVariant();
}

QModelIndex
CategoryModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row, column);
}

QModelIndex
CategoryModel::parent(const QModelIndex& /*index*/) const
{
	return QModelIndex();
}

Qt::ItemFlags
CategoryModel::flags(const QModelIndex& index) const
{
	if (!index.isValid()) {
		return Qt::ItemIsEnabled;
	}

	if (index.column() == 0) {
		return QAbstractItemModel::flags(index);//Qt::ItemIsUserCheckable;
	}
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool
CategoryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (model_ == nullptr || !index.isValid() || role != Qt::EditRole) {
		return false;
	}
	unsigned int row = index.row();
	if (row >= model_->categoryList().size()) {
		return false;
	}

	if (index.column() == 1) {
		std::string name = value.toString().toStdString();
		if (model_->categoryList()[row]->name() == name) {
			return false;
		}
		if (model_->findCategoryName(name)) {
			emit errorOccurred(tr("Duplicate category: %1").arg(name.c_str()));
			return false;
		}
		model_->categoryList()[row]->setName(name);
		emit dataChanged(index, index);
		emit categoryChanged();
		return true;
	}
	return false;
}

QVariant
CategoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if (orientation == Qt::Horizontal) {
		switch (section) {
		case 0:
			return QVariant(tr("Is used?"));
		case 1:
			return QVariant(tr("Name"));
		default:
			return QVariant();
		}
	} else {
		return QVariant(section + 1);
	}
}

bool
CategoryModel::insertRows(int row, int count, const QModelIndex& /*parent*/)
{
	if (model_ == nullptr || count != 1) {
		return false;
	}
	if (row < 0 || static_cast<std::size_t>(row) > model_->categoryList().size()) {
		return false;
	}

	if (model_->findCategoryName(NEW_ITEM_NAME)) {
		emit errorOccurred(tr("Duplicate category: %1").arg(NEW_ITEM_NAME));
		return false;
	}

	beginInsertRows(QModelIndex(), row, row);
	auto newCategory = std::make_shared<VTMControlModel::Category>(NEW_ITEM_NAME);
	model_->categoryList().insert(
				model_->categoryList().begin() + row,
				newCategory);
	endInsertRows();

	emit categoryChanged();

	return true;
}

bool
CategoryModel::removeRows(int row, int count, const QModelIndex& /*parent*/)
{
	if (model_ == nullptr || count != 1) {
		return false;
	}
	if (row < 0 || static_cast<std::size_t>(row) >= model_->categoryList().size()) {
		return false;
	}
	if (model_->categoryList()[row].use_count() > 1) {
		emit errorOccurred(tr("Can't remove category in use."));
		return false;
	}

	beginRemoveRows(QModelIndex(), row, row);
	model_->categoryList().erase(model_->categoryList().begin() + row);
	endRemoveRows();

	emit categoryChanged();

	return true;
}

void
CategoryModel::resetModel(VTMControlModel::Model* model)
{
	beginResetModel();
	model_ = model;
	endResetModel();
}

QString
CategoryModel::getCategoryComment(const QModelIndex& index) const
{
	if (model_ == nullptr || !index.isValid()) {
		return QString();
	}
	unsigned int row = index.row();
	if (row >= model_->categoryList().size()) {
		return QString();
	}
	return QString::fromStdString(model_->categoryList()[row]->comment());
}

void
CategoryModel::setCategoryComment(const QModelIndex& index, const QString& comment) const
{
	if (!index.isValid()) {
		return;
	}
	unsigned int row = index.row();
	if (row >= model_->categoryList().size()) {
		return;
	}

	model_->categoryList()[row]->setComment(comment.toStdString());
}

// Returns the new index.
QModelIndex
CategoryModel::incrementCategoryRow(const QModelIndex& index)
{
	if (model_ == nullptr || !index.isValid()) {
		return index;
	}

	unsigned int row = index.row();
	if (row < model_->categoryList().size() - 1U) {
		std::swap(model_->categoryList()[row], model_->categoryList()[row + 1]);
		emit dataChanged(createIndex(row, 0 /* first column */), createIndex(row + 1, NUM_COLUMNS - 1 /* last column */));
		emit categoryChanged();
		return createIndex(row + 1, NUM_COLUMNS - 1 /* last column */);
	}
	return index;
}

// Returns the new index.
QModelIndex
CategoryModel::decrementCategoryRow(const QModelIndex& index)
{
	if (model_ == nullptr || !index.isValid()) {
		return index;
	}

	unsigned int row = index.row();
	if (row > 0) {
		std::swap(model_->categoryList()[row - 1], model_->categoryList()[row]);
		emit dataChanged(createIndex(row - 1, 0 /* first column */), createIndex(row, NUM_COLUMNS - 1 /* last column */));
		emit categoryChanged();
		return createIndex(row - 1, NUM_COLUMNS - 1 /* last column */);
	}
	return index;
}

void
CategoryModel::updateView()
{
	beginResetModel();
	endResetModel();
}

} // namespace GS

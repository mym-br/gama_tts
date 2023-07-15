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

#include "DataEntryWindow.h"

#include <QMessageBox>

#include "CategoryModel.h"
#include "Model.h"
#include "ParameterModel.h"
#include "SymbolModel.h"
#include "ui_DataEntryWindow.h"



namespace GS {

DataEntryWindow::DataEntryWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::DataEntryWindow>())
		, categoryModel_(new CategoryModel(this))
		, parameterModel_(new ParameterModel(this))
		, symbolModel_(new SymbolModel(this))
{
	ui_->setupUi(this);

	QFontMetrics fm = fontMetrics();
	int rowHeight = fm.height() + fm.ascent();

	QHeaderView* vHeader = ui_->categoriesTableView->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->categoriesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui_->categoriesTableView->setModel(categoryModel_);

	vHeader = ui_->parametersTableView->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->parametersTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui_->parametersTableView->setModel(parameterModel_);

	vHeader = ui_->symbolsTableView->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->symbolsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui_->symbolsTableView->setModel(symbolModel_);

	connect(ui_->categoriesTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
			this, &DataEntryWindow::showCategoryData);
	connect(ui_->parametersTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
			this, &DataEntryWindow::showParameterData);
	connect(ui_->symbolsTableView->selectionModel()   , &QItemSelectionModel::currentRowChanged,
			this, &DataEntryWindow::showSymbolData);

	connect(categoryModel_, &CategoryModel::categoryChanged, this, &DataEntryWindow::categoryChanged);
	connect(categoryModel_, &CategoryModel::errorOccurred  , this, &DataEntryWindow::showError);

	connect(parameterModel_, &ParameterModel::parameterChanged, this, &DataEntryWindow::parameterChanged);
	connect(parameterModel_, &ParameterModel::errorOccurred   , this, &DataEntryWindow::showError);

	connect(symbolModel_, &SymbolModel::symbolChanged, this, &DataEntryWindow::symbolChanged);
	connect(symbolModel_, &SymbolModel::errorOccurred, this, &DataEntryWindow::showError);
}

DataEntryWindow::~DataEntryWindow()
{
}

void
DataEntryWindow::resetModel(VTMControlModel::Model* model)
{
	categoryModel_->resetModel(model);
	if (categoryModel_->rowCount() > 0) {
		ui_->categoriesTableView->setCurrentIndex(categoryModel_->index(0, CategoryModel::NUM_COLUMNS - 1));
	}

	parameterModel_->resetModel(model);
	if (parameterModel_->rowCount() > 0) {
		ui_->parametersTableView->setCurrentIndex(parameterModel_->index(0, ParameterModel::NUM_COLUMNS - 1));
	}

	symbolModel_->resetModel(model);
	if (symbolModel_->rowCount() > 0) {
		ui_->symbolsTableView->setCurrentIndex(symbolModel_->index(0, SymbolModel::NUM_COLUMNS - 1));
	}
}

void
DataEntryWindow::updateCategoriesTable()
{
	categoryModel_->updateView();
}

//=============================================================================
// Categories.
//

void
DataEntryWindow::on_moveCategoryDownButton_clicked()
{
	QModelIndex index = categoryModel_->incrementCategoryRow(ui_->categoriesTableView->currentIndex());
	ui_->categoriesTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_moveCategoryUpButton_clicked()
{
	QModelIndex index = categoryModel_->decrementCategoryRow(ui_->categoriesTableView->currentIndex());
	ui_->categoriesTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_addCategoryButton_clicked()
{
	QModelIndex curIndex = ui_->categoriesTableView->currentIndex();
	int insertPos;
	if (curIndex.isValid()) {
		insertPos = curIndex.row() + 1;
	} else {
		insertPos = categoryModel_->rowCount();
	}
	if (categoryModel_->insertRows(insertPos, 1)) {
		ui_->categoriesTableView->setCurrentIndex(categoryModel_->index(insertPos, CategoryModel::NUM_COLUMNS - 1));
	}
}

void
DataEntryWindow::on_removeCategoryButton_clicked()
{
	QModelIndex curIndex = ui_->categoriesTableView->currentIndex();
	if (curIndex.isValid()) {
		categoryModel_->removeRows(curIndex.row(), 1);
	}
}

// Slot.
void
DataEntryWindow::showCategoryData(const QModelIndex& current, const QModelIndex& /*previous*/)
{
	ui_->categoryCommentTextEdit->setPlainText(categoryModel_->getCategoryComment(current));
}

void
DataEntryWindow::on_updateCategoryCommentButton_clicked()
{
	categoryModel_->setCategoryComment(ui_->categoriesTableView->currentIndex(), ui_->categoryCommentTextEdit->toPlainText());
}

//=============================================================================
// Parameters.
//

void
DataEntryWindow::on_moveParameterDownButton_clicked()
{
	QModelIndex index = parameterModel_->incrementParameterRow(ui_->parametersTableView->currentIndex());
	ui_->parametersTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_moveParameterUpButton_clicked()
{
	QModelIndex index = parameterModel_->decrementParameterRow(ui_->parametersTableView->currentIndex());
	ui_->parametersTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_addParameterButton_clicked()
{
	QModelIndex curIndex = ui_->parametersTableView->currentIndex();
	int insertPos;
	if (curIndex.isValid()) {
		insertPos = curIndex.row() + 1;
	} else {
		insertPos = parameterModel_->rowCount();
	}
	if (parameterModel_->insertRows(insertPos, 1)) {
		ui_->parametersTableView->setCurrentIndex(parameterModel_->index(insertPos, ParameterModel::NUM_COLUMNS - 1));
	}
}

void
DataEntryWindow::on_removeParameterButton_clicked()
{
	QModelIndex curIndex = ui_->parametersTableView->currentIndex();
	if (curIndex.isValid()) {
		parameterModel_->removeRows(curIndex.row(), 1);
	}
}

// Slot.
void
DataEntryWindow::showParameterData(const QModelIndex& current, const QModelIndex& /*previous*/)
{
	ui_->parameterCommentTextEdit->setPlainText(parameterModel_->getParameterComment(current));
}

void
DataEntryWindow::on_updateParameterCommentButton_clicked()
{
	parameterModel_->setParameterComment(ui_->parametersTableView->currentIndex(), ui_->parameterCommentTextEdit->toPlainText());
}

//=============================================================================
// Symbols.
//

void
DataEntryWindow::on_moveSymbolDownButton_clicked()
{
	QModelIndex index = symbolModel_->incrementSymbolRow(ui_->symbolsTableView->currentIndex());
	ui_->symbolsTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_moveSymbolUpButton_clicked()
{
	QModelIndex index = symbolModel_->decrementSymbolRow(ui_->symbolsTableView->currentIndex());
	ui_->symbolsTableView->setCurrentIndex(index);
}

void
DataEntryWindow::on_addSymbolButton_clicked()
{
	QModelIndex curIndex = ui_->symbolsTableView->currentIndex();
	int insertPos;
	if (curIndex.isValid()) {
		insertPos = curIndex.row() + 1;
	} else {
		insertPos = symbolModel_->rowCount();
	}
	if (symbolModel_->insertRows(insertPos, 1)) {
		ui_->symbolsTableView->setCurrentIndex(symbolModel_->index(insertPos, SymbolModel::NUM_COLUMNS - 1));
	}
}

void
DataEntryWindow::on_removeSymbolButton_clicked()
{
	QModelIndex curIndex = ui_->symbolsTableView->currentIndex();
	if (curIndex.isValid()) {
		symbolModel_->removeRows(curIndex.row(), 1);
	}
}

// Slot.
void
DataEntryWindow::showSymbolData(const QModelIndex& current, const QModelIndex& /*previous*/)
{
	ui_->symbolCommentTextEdit->setPlainText(symbolModel_->getSymbolComment(current));
}

void
DataEntryWindow::on_updateSymbolCommentButton_clicked()
{
	symbolModel_->setSymbolComment(ui_->symbolsTableView->currentIndex(), ui_->symbolCommentTextEdit->toPlainText());
}

// Slot.
void
DataEntryWindow::showError(QString msg)
{
	QMessageBox::critical(this, tr("Error"), msg);
}

} // namespace GS

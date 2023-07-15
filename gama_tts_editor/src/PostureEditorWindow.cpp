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

#include "PostureEditorWindow.h"

#include <algorithm> /* find_if */
#include <memory>
#include <string>
#include <utility> /* move */

#include <QMessageBox>
#include <QSignalBlocker>

#include "Clipboard.h"
#include "Model.h"
#include "ui_PostureEditorWindow.h"

#define NUM_POSTURES_TABLE_COLUMNS 1
#define NUM_CATEGORIES_TABLE_COLUMNS 2
#define NUM_PARAMETERS_TABLE_COLUMNS 5
#define NUM_SYMBOLS_TABLE_COLUMNS 5
#define NEW_ITEM_NAME "###new###"



namespace GS {

PostureEditorWindow::PostureEditorWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::PostureEditorWindow>())
		, model_()
		, postureNameRegExp_(QRegularExpression::anchoredPattern("[^\\x{0000}-\\x{0020}\\x{007f}'_*./0-9]+"))
{
	ui_->setupUi(this);

	QFontMetrics fm = fontMetrics();
	int rowHeight = fm.height() + fm.ascent();

	QHeaderView* vHeader = ui_->posturesTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->posturesTable->setColumnCount(NUM_POSTURES_TABLE_COLUMNS);
	ui_->posturesTable->setHorizontalHeaderLabels(QStringList() << tr("Posture"));
	ui_->posturesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->categoriesTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->categoriesTable->setColumnCount(NUM_CATEGORIES_TABLE_COLUMNS);
	ui_->categoriesTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Is member?"));
	ui_->categoriesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->parametersTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->parametersTable->setColumnCount(NUM_PARAMETERS_TABLE_COLUMNS);
	ui_->parametersTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value") << tr("Minimum") << tr("Maximum") << tr("Default"));
	ui_->parametersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	vHeader = ui_->symbolsTable->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->symbolsTable->setColumnCount(NUM_SYMBOLS_TABLE_COLUMNS);
	ui_->symbolsTable->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value") << tr("Minimum") << tr("Maximum") << tr("Default"));
	ui_->symbolsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

PostureEditorWindow::~PostureEditorWindow()
{
}

void
PostureEditorWindow::resetModel(VTMControlModel::Model* model)
{
	model_ = model;

	if (model_ == nullptr) {
		clearPostureData();
		ui_->posturesTable->setRowCount(0);
	} else {
		setupPosturesTable();
	}
}

// Slot.
void
PostureEditorWindow::unselectPosture()
{
	ui_->posturesTable->setCurrentItem(nullptr);
}

void
PostureEditorWindow::on_addPostureButton_clicked()
{
	if (model_ == nullptr) return;

	if (model_->postureList().find(NEW_ITEM_NAME)) {
		QMessageBox::critical(this, tr("Error"), tr("Duplicate posture name: %1").arg(NEW_ITEM_NAME));
		return;
	}

	auto newPosture = std::make_unique<VTMControlModel::Posture>(
					NEW_ITEM_NAME,
					model_->parameterList().size(),
					model_->symbolList().size());
	std::shared_ptr<VTMControlModel::Category> cat = model_->findCategory("phone"); // hardcoded
	if (!cat) {
		QMessageBox::critical(this, tr("Error"), tr("Category not found: phone."));
		return;
	}
	newPosture->categoryList().push_back(cat);
	for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
		const auto& parameter = model_->parameterList()[i];
		newPosture->setParameterTarget(i, parameter.defaultValue());
	}
	for (unsigned int i = 0, size = model_->symbolList().size(); i < size; ++i) {
		const auto& symbol = model_->symbolList()[i];
		newPosture->setSymbolTarget(i, symbol.defaultValue());
	}
	model_->postureList().add(std::move(newPosture));

	setupPosturesTable();

	emit postureChanged();
}

void
PostureEditorWindow::on_removePostureButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;

	int currPostureRow = currPostureItem->row();
	model_->postureList().remove(currPostureRow);

	setupPosturesTable();

	emit postureChanged();
}

void
PostureEditorWindow::on_updatePostureCommentButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;

	int currPostureRow = currPostureItem->row();
	VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];
	posture.setComment(ui_->postureCommentTextEdit->toPlainText().toStdString());
}

void
PostureEditorWindow::on_posturesTable_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* /*previous*/)
{
	if (model_ == nullptr) return;
	if (current == nullptr) {
		clearPostureData();
		return;
	}

	unsigned int row = current->row();
	const VTMControlModel::Posture& posture = model_->postureList()[row];
	ui_->postureCommentTextEdit->setPlainText(posture.comment().c_str());
	setupCategoriesTable(posture);
	setupParametersTable(posture);
	setupSymbolsTable(posture);

	// Send the parameters to InteractiveVTMWindow.
	QHash<QString, float> paramMap;
	fillParametersMap(paramMap);
	if (!paramMap.empty()) {
		emit currentPostureChanged(paramMap);
	}
}

void
PostureEditorWindow::on_posturesTable_itemChanged(QTableWidgetItem* item)
{
	qDebug("on_posturesTable_itemChanged");

	if (model_ == nullptr) return;

	int row = item->row();
	auto restoreName = [&]() {
		QSignalBlocker blocker(ui_->posturesTable);
		item->setText(model_->postureList()[row].name().c_str());
	};

	const std::string newName = item->text().toStdString();
	if (!postureNameRegExp_.match(item->text()).hasMatch()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid posture name."));
		restoreName();
	} else if (model_->postureList().find(newName)) {
		QMessageBox::critical(this, tr("Error"), tr("Duplicate posture name."));
		restoreName();
	} else if (model_->findCategoryName(newName)) {
		QMessageBox::critical(this, tr("Error"), tr("Postures can not have the name of a category."));
		restoreName();
	} else {
		const auto& posture = model_->postureList()[row];
		std::unique_ptr<VTMControlModel::Posture> newPosture = posture.copy(newName);
		model_->postureList().remove(row);
		model_->postureList().add(std::move(newPosture));

		setupPosturesTable();

		emit postureChanged();
	}
}

void
PostureEditorWindow::on_categoriesTable_itemChanged(QTableWidgetItem* item)
{
	qDebug("on_categoriesTable_itemChanged");

	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();

	int row = item->row();
	int col = item->column();
	if (col == 1) {
		VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];
		std::shared_ptr<VTMControlModel::Category> category = model_->categoryList()[row];
		if (item->checkState() == Qt::Checked) {
			posture.categoryList().push_back(category);
		} else {
			auto iter = std::find_if(
					posture.categoryList().begin(), posture.categoryList().end(),
					[&category](const std::shared_ptr<VTMControlModel::Category>& c) {
						return c->name() == category->name();
					});
			if (iter != posture.categoryList().end()) {
				posture.categoryList().erase(iter);
			}
		}
		emit postureCategoryChanged();
	}
}

void
PostureEditorWindow::on_parametersTable_itemChanged(QTableWidgetItem* item)
{
	qDebug("on_parametersTable_itemChanged");

	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();

	int row = item->row();
	int col = item->column();
	if (col == 1) {
		VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];
		const VTMControlModel::Parameter& parameter = model_->parameterList()[row];

		bool ok;
		float value = item->data(Qt::DisplayRole).toFloat(&ok);
		bool updateValue = true, valueChanged = false;
		if (!ok) {
			value = posture.getParameterTarget(row);
			updateValue = false;
			valueChanged = true;
		}
		if (value < parameter.minimum()) {
			value = parameter.minimum();
			updateValue = true;
			valueChanged = true;
		} else if (value > parameter.maximum()) {
			value = parameter.maximum();
			updateValue = true;
			valueChanged = true;
		}
		if (updateValue) {
			posture.setParameterTarget(row, value);
		}

		QSignalBlocker blocker(ui_->parametersTable);
		if (valueChanged) {
			item->setData(Qt::DisplayRole, value);
		}
		if (value != parameter.defaultValue()) {
			QFont boldFont = font();
			boldFont.setBold(true);
			item->setData(Qt::FontRole, boldFont);
		} else {
			item->setData(Qt::FontRole, font());
		}
	}
}

void
PostureEditorWindow::on_useDefaultParameterValueButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();
	VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];

	QTableWidgetItem* item = ui_->parametersTable->currentItem();
	if (item == nullptr) return;
	int row = item->row();
	const VTMControlModel::Parameter& parameter = model_->parameterList()[row];

	posture.setParameterTarget(row, parameter.defaultValue());
	setupParametersTable(posture);
}

void
PostureEditorWindow::on_symbolsTable_itemChanged(QTableWidgetItem* item)
{
	qDebug("on_symbolsTable_itemChanged");

	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();

	int row = item->row();
	int col = item->column();
	if (col == 1) {
		VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];
		const VTMControlModel::Symbol& symbol = model_->symbolList()[row];

		bool ok;
		float value = item->data(Qt::DisplayRole).toFloat(&ok);
		bool updateValue = true, valueChanged = false;
		if (!ok) {
			value = posture.getSymbolTarget(row);
			updateValue = false;
			valueChanged = true;
		}
		if (value < symbol.minimum()) {
			value = symbol.minimum();
			updateValue = true;
			valueChanged = true;
		} else if (value > symbol.maximum()) {
			value = symbol.maximum();
			updateValue = true;
			valueChanged = true;
		}
		if (updateValue) {
			posture.setSymbolTarget(row, value);
		}

		QSignalBlocker blocker(ui_->symbolsTable);
		if (valueChanged) {
			item->setData(Qt::DisplayRole, value);
		}
		if (value != symbol.defaultValue()) {
			QFont boldFont = font();
			boldFont.setBold(true);
			item->setData(Qt::FontRole, boldFont);
		} else {
			item->setData(Qt::FontRole, font());
		}
	}
}

void
PostureEditorWindow::on_useDefaultSymbolValueButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();
	VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];

	QTableWidgetItem* item = ui_->symbolsTable->currentItem();
	if (item == nullptr) return;
	int row = item->row();
	const VTMControlModel::Symbol& symbol = model_->symbolList()[row];

	posture.setSymbolTarget(row, symbol.defaultValue());
	setupSymbolsTable(posture);
}

void
PostureEditorWindow::fillParametersMap(QHash<QString, float>& paramMap)
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();
	const VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];

	for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
		const auto& parameter = model_->parameterList()[i];
		const float value = posture.getParameterTarget(i);
		paramMap[parameter.name().c_str()] = value;
	}
}

void
PostureEditorWindow::on_copyParametersButton_clicked()
{
	QHash<QString, float> paramMap;
	fillParametersMap(paramMap);
	if (!paramMap.empty()) {
		Clipboard::putPostureParameters(paramMap);
	}
}

void
PostureEditorWindow::on_pasteParametersButton_clicked()
{
	if (model_ == nullptr) return;

	QTableWidgetItem* currPostureItem = ui_->posturesTable->currentItem();
	if (currPostureItem == nullptr) return;
	int currPostureRow = currPostureItem->row();
	VTMControlModel::Posture& posture = model_->postureList()[currPostureRow];

	QHash<QString, float> paramMap = Clipboard::getPostureParameters();
	if (paramMap.empty()) return;

	for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
		const auto& parameter = model_->parameterList()[i];
		QString name(parameter.name().c_str());
		auto iter = paramMap.find(name.replace(' ', '_'));
		if (iter != paramMap.end()) {
			float value = qBound(parameter.minimum(), iter.value(), parameter.maximum());
			posture.setParameterTarget(i, value);
		}
	}

	setupParametersTable(posture);
}

void
PostureEditorWindow::setupPosturesTable()
{
	if (model_ == nullptr) return;

	QTableWidget* table = ui_->posturesTable;
	{
		QSignalBlocker blocker(table);

		table->setRowCount(model_->postureList().size());
		for (unsigned int i = 0, size = model_->postureList().size(); i < size; ++i) {
			const auto& posture = model_->postureList()[i];

			auto item = std::make_unique<QTableWidgetItem>(posture.name().c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
			table->setItem(i, 0, item.release());
		}
	}
	table->setCurrentItem(nullptr);
	clearPostureData();
}

void
PostureEditorWindow::clearPostureData()
{
	ui_->postureCommentTextEdit->clear();
	ui_->categoriesTable->setRowCount(0);
	ui_->parametersTable->setRowCount(0);
	ui_->symbolsTable->setRowCount(0);
}

void
PostureEditorWindow::setupCategoriesTable(const VTMControlModel::Posture& posture)
{
	QTableWidget* table = ui_->categoriesTable;
	{
		QSignalBlocker blocker(table);

		table->setRowCount(model_->categoryList().size());
		for (unsigned int i = 0, size = model_->categoryList().size(); i < size; ++i) {
			const auto& category = model_->categoryList()[i];

			auto item = std::make_unique<QTableWidgetItem>(category->name().c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(i, 0, item.release());

			item = std::make_unique<QTableWidgetItem>();
			if (category->name() == "phone") { // hardcoded
				item->setFlags(Qt::ItemIsSelectable);
			} else {
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			}
			bool useCategory = false;
			for (auto& postureCategory : posture.categoryList()) {
				if (category->name() == postureCategory->name()) {
					useCategory = true;
					break;
				}
			}
			item->setCheckState(useCategory ? Qt::Checked : Qt::Unchecked);
			table->setItem(i, 1, item.release());
		}
	}
}

void
PostureEditorWindow::setupParametersTable(const VTMControlModel::Posture& posture)
{
	QTableWidget* table = ui_->parametersTable;
	{
		QSignalBlocker blocker(table);

		QFont boldFont = font();
		boldFont.setBold(true);

		table->setRowCount(model_->parameterList().size());
		for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
			const auto& parameter = model_->parameterList()[i];

			float value = posture.getParameterTarget(i);
			float defaultValue = parameter.defaultValue();
			bool usingDefaultValue = (value == defaultValue);

			// Name.
			auto item = std::make_unique<QTableWidgetItem>(parameter.name().c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(i, 0, item.release());

			// Value.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			if (!usingDefaultValue) item->setData(Qt::FontRole, boldFont);
			item->setData(Qt::DisplayRole, value);
			table->setItem(i, 1, item.release());

			// Min.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, parameter.minimum());
			table->setItem(i, 2, item.release());

			// Max.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, parameter.maximum());
			table->setItem(i, 3, item.release());

			// Default.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, defaultValue);
			table->setItem(i, 4, item.release());
		}
	}
}

void
PostureEditorWindow::setupSymbolsTable(const VTMControlModel::Posture& posture)
{
	QTableWidget* table = ui_->symbolsTable;
	{
		QSignalBlocker blocker(table);

		QFont boldFont = font();
		boldFont.setBold(true);

		table->setRowCount(model_->symbolList().size());
		for (unsigned int i = 0, size = model_->symbolList().size(); i < size; ++i) {
			const auto& symbol = model_->symbolList()[i];

			float value = posture.getSymbolTarget(i);
			float defaultValue = symbol.defaultValue();
			bool usingDefaultValue = (value == defaultValue);

			// Name.
			auto item = std::make_unique<QTableWidgetItem>(symbol.name().c_str());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			table->setItem(i, 0, item.release());

			// Value.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
			if (!usingDefaultValue) item->setData(Qt::FontRole, boldFont);
			item->setData(Qt::DisplayRole, value);
			table->setItem(i, 1, item.release());

			// Min.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, symbol.minimum());
			table->setItem(i, 2, item.release());

			// Max.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, symbol.maximum());
			table->setItem(i, 3, item.release());

			// Default.
			item = std::make_unique<QTableWidgetItem>();
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item->setData(Qt::DisplayRole, defaultValue);
			table->setItem(i, 4, item.release());
		}
	}
}

} // namespace GS

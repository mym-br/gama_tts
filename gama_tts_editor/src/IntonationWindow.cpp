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

#include "IntonationWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QStringList>

#include "Controller.h"
#include "Synthesis.h"
#include "ui_IntonationWindow.h"



namespace GS {

IntonationWindow::IntonationWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::IntonationWindow>())
		, synthesis_()
{
	ui_->setupUi(this);

	ui_->intonationScrollArea->setBackgroundRole(QPalette::Base);

	connect(ui_->intonationWidget, &IntonationWidget::pointSelected, this, &IntonationWindow::setPointData);
}

IntonationWindow::~IntonationWindow()
{
}

void
IntonationWindow::clear()
{
	ui_->intonationWidget->updateData(nullptr);
	synthesis_ = nullptr;
}

void
IntonationWindow::setup(Synthesis* synthesis)
{
	if (synthesis == nullptr) {
		clear();
		return;
	}

	synthesis_ = synthesis;

	ui_->intonationWidget->updateData(&synthesis_->vtmController->eventList());
}

void
IntonationWindow::on_synthesizeButton_clicked()
{
	qDebug("IntonationWindow::on_synthesizeButton_clicked");

	if (synthesis_ == nullptr) return;

	if (!ui_->intonationWidget->saveIntonationToEventList()) {
		return;
	}

	disableProcessingButtons();

	emit synthesisRequested();
}

void
IntonationWindow::on_synthesizeToFileButton_clicked()
{
	qDebug("IntonationWindow::on_synthesizeToFileButton_clicked");

	if (synthesis_ == nullptr) return;

	QString filePath = QFileDialog::getSaveFileName(this, tr("Save file"), synthesis_->appConfig.projectDir, tr("WAV files (*.wav)"));
	if (filePath.isEmpty()) {
		return;
	}

	if (!ui_->intonationWidget->saveIntonationToEventList()) {
		return;
	}

	disableProcessingButtons();

	emit synthesisToFileRequested(filePath);
}

// Slot.
void
IntonationWindow::loadIntonationFromEventList()
{
	ui_->intonationWidget->loadIntonationFromEventList();
}

void
IntonationWindow::on_valueLineEdit_editingFinished()
{
	bool ok;
	double value = ui_->valueLineEdit->text().toDouble(&ok);
	if (!ok) {
		ui_->intonationWidget->sendSelectedPointData();
		return;
	}
	ui_->intonationWidget->setSelectedPointValue(value);
}

void
IntonationWindow::on_slopeLineEdit_editingFinished()
{
	bool ok;
	double slope = ui_->slopeLineEdit->text().toDouble(&ok);
	if (!ok) {
		ui_->intonationWidget->sendSelectedPointData();
		return;
	}
	ui_->intonationWidget->setSelectedPointSlope(slope);
}

void
IntonationWindow::on_beatOffsetLineEdit_editingFinished()
{
	bool ok;
	double beatOffset = ui_->beatOffsetLineEdit->text().toDouble(&ok);
	if (!ok) {
		ui_->intonationWidget->sendSelectedPointData();
		return;
	}
	ui_->intonationWidget->setSelectedPointBeatOffset(beatOffset);
}

// Slot.
void
IntonationWindow::setPointData(double value, double slope, double beat, double beatOffset, double absoluteTime)
{
	ui_->valueLineEdit->setText(QString::number(value));
	ui_->slopeLineEdit->setText(QString::number(slope));
	ui_->beatLineEdit->setText(QString::number(beat));
	ui_->beatOffsetLineEdit->setText(QString::number(beatOffset));
	ui_->absoluteTimeLineEdit->setText(QString::number(absoluteTime));
}

// Slot.
void
IntonationWindow::enableProcessingButtons()
{
	ui_->synthesizeButton->setEnabled(true);
	ui_->synthesizeToFileButton->setEnabled(true);
}

// Slot.
void
IntonationWindow::disableProcessingButtons()
{
	ui_->synthesizeButton->setEnabled(false);
	ui_->synthesizeToFileButton->setEnabled(false);
}

} // namespace GS

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

#include "SynthesisWindow.h"

#include <cmath> /* rint */
#include <memory>
#include <string>

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QScrollBar>
#include <QString>
#include <QStringList>

#include "AudioWorker.h"
#include "Controller.h"
#include "Index.h"
#include "Model.h"
#include "PhoneticStringParser.h"
#include "Synthesis.h"
#include "TextParser.h"
#include "ui_SynthesisWindow.h"

#define VTM_PARAM_FILE_NAME "generated__vtm_param.txt"



namespace GS {

SynthesisWindow::SynthesisWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::SynthesisWindow>())
		, model_()
		, synthesis_()
		, audioWorker_()
		, speechSamplerate_()
{
	ui_->setupUi(this);

	QFontMetrics fm = fontMetrics();
	int rowHeight = fm.height() + fm.ascent();

	QHeaderView* vHeader = ui_->parameterTableWidget->verticalHeader();
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setDefaultSectionSize(rowHeight);
	ui_->parameterTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui_->parameterTableWidget->setColumnCount(1);
	ui_->parameterTableWidget->setHorizontalHeaderLabels(QStringList() << tr("Parameter"));

	ui_->tempoSpinBox->setRange(0.1, 10.0);
	ui_->tempoSpinBox->setSingleStep(0.1);
	ui_->tempoSpinBox->setDecimals(1);
	ui_->tempoSpinBox->setValue(1.0);

	ui_->xZoomSpinBox->setRange(ui_->parameterWidget->xZoomMin(), ui_->parameterWidget->xZoomMax());
	ui_->xZoomSpinBox->setSingleStep(0.1);
	ui_->xZoomSpinBox->setValue(1.0);

	ui_->yZoomSpinBox->setRange(ui_->parameterWidget->yZoomMin(), ui_->parameterWidget->yZoomMax());
	ui_->yZoomSpinBox->setSingleStep(0.1);
	ui_->yZoomSpinBox->setValue(1.0);

	ui_->parameterScrollArea->setBackgroundRole(QPalette::Base);

	connect(ui_->textLineEdit   , &QLineEdit::returnPressed   , ui_->parseButton, &QPushButton::click);
	connect(ui_->parameterWidget, &ParameterWidget::mouseMoved, this            , &SynthesisWindow::updateMouseTracking);
	connect(ui_->parameterWidget, &ParameterWidget::zoomReset , this            , &SynthesisWindow::resetZoom);
	connect(ui_->parameterScrollArea->verticalScrollBar()  , &QScrollBar::valueChanged, ui_->parameterWidget, &ParameterWidget::getVerticalScrollbarValue);
	connect(ui_->parameterScrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, ui_->parameterWidget, &ParameterWidget::getHorizontalScrollbarValue);

	audioWorker_ = new AudioWorker;
	audioWorker_->moveToThread(&audioThread_);
	connect(&audioThread_, &QThread::finished,
			audioWorker_, &AudioWorker::deleteLater);
	connect(this         , &SynthesisWindow::playAudioRequested,
			audioWorker_, &AudioWorker::playAudio);
	connect(audioWorker_ , &AudioWorker::finished,
			this        , &SynthesisWindow::handleAudioFinished);
	connect(audioWorker_ , &AudioWorker::errorOccurred,
			this        , &SynthesisWindow::handleAudioError);
	audioThread_.start();
}

SynthesisWindow::~SynthesisWindow()
{
	audioThread_.quit();
	audioThread_.wait();
}

void
SynthesisWindow::clear()
{
	ui_->parameterTableWidget->setRowCount(0);
	ui_->parameterWidget->updateData(nullptr, nullptr, nullptr, nullptr);
	synthesis_ = nullptr;
	model_ = nullptr;
}

void
SynthesisWindow::setup(VTMControlModel::Model* model, Synthesis* synthesis)
{
	if (!model || !synthesis) {
		clear();
		return;
	}

	model_ = model;
	synthesis_ = synthesis;

	setupParameterWidget(false);
}

void
SynthesisWindow::on_parseButton_clicked()
{
	if (!synthesis_) {
		return;
	}
	QString text = ui_->textLineEdit->text();
	if (text.trimmed().isEmpty()) {
		return;
	}
	disableProcessingButtons();

	try {
		Index index{synthesis_->appConfig.projectDir.toStdString()};
		auto textParser = TextParser::TextParser::getInstance(
					index,
					synthesis_->vtmController->vtmControlModelConfiguration().phoStrFormat);
		std::string phoneticString = textParser->parse(text.toUtf8().constData());
		ui_->phoneticStringTextEdit->setPlainText(phoneticString.c_str());
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		enableProcessingButtons();
		return;
	}

	on_synthesizeButton_clicked();
}

void
SynthesisWindow::on_referenceButton_clicked()
{
	if (!synthesis_) {
		return;
	}
	QString phoneticString = ui_->phoneticStringTextEdit->toPlainText();
	if (phoneticString.trimmed().isEmpty()) {
		return;
	}

	emit synthesisStarted();
	disableProcessingButtons();

	try {
		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		synthesis_->refModel = std::make_unique<VTMControlModel::Model>();
		synthesis_->refModel->load(synthesis_->appConfig.dataFilePath.toStdString());
		if (synthesis_->refModel->parameterList().size() != model_->parameterList().size()) {
			QMessageBox::critical(this, tr("Error"), "The reference model has not the same number of parameters as the current model.");
			enableProcessingButtons();
			emit synthesisFinished();
			return;
		}

		synthesis_->refVtmController = std::make_unique<VTMControlModel::Controller>(
							*synthesis_->index,
							*synthesis_->refModel);
		VTMControlModel::Configuration& config = synthesis_->refVtmController->vtmControlModelConfiguration();
		config.tempo = ui_->tempoSpinBox->value();

		audioWorker_->player().fillBuffer([&](std::vector<float>& buffer) {
			synthesis_->refVtmController->synthesizePhoneticStringToBuffer(
								phoneticString.toStdString(),
								saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
								buffer);
		});

		setupParameterWidget(true);

		emit playAudioRequested(synthesis_->refVtmController->outputSampleRate());
		emit textSynthesized();
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		enableProcessingButtons();
		emit synthesisFinished();
	}
}

void
SynthesisWindow::on_synthesizeButton_clicked()
{
	if (!synthesis_) {
		return;
	}
	QString phoneticString = ui_->phoneticStringTextEdit->toPlainText();
	if (phoneticString.trimmed().isEmpty()) {
		return;
	}

	emit synthesisStarted();
	disableProcessingButtons();

	try {
		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		VTMControlModel::Configuration& config = synthesis_->vtmController->vtmControlModelConfiguration();
		config.tempo = ui_->tempoSpinBox->value();

		audioWorker_->player().fillBuffer([&](std::vector<float>& buffer) {
			synthesis_->vtmController->synthesizePhoneticStringToBuffer(
							phoneticString.toStdString().c_str(),
							saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
							buffer);
		});

		setupParameterWidget(false);

		emit playAudioRequested(synthesis_->vtmController->outputSampleRate());
		emit textSynthesized();
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		enableProcessingButtons();
		emit synthesisFinished();
	}
}

void
SynthesisWindow::on_synthesizeToFileButton_clicked()
{
	if (!synthesis_) {
		return;
	}
	QString phoneticString = ui_->phoneticStringTextEdit->toPlainText();
	if (phoneticString.trimmed().isEmpty()) {
		return;
	}
	QString filePath = QFileDialog::getSaveFileName(this, tr("Save file"), synthesis_->appConfig.projectDir, tr("WAV files (*.wav)"));
	if (filePath.isEmpty()) {
		return;
	}

	emit synthesisStarted();
	disableProcessingButtons();

	try {
		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		VTMControlModel::Configuration& config = synthesis_->vtmController->vtmControlModelConfiguration();
		config.tempo = ui_->tempoSpinBox->value();

		synthesis_->vtmController->synthesizePhoneticStringToFile(
						phoneticString.toStdString(),
						saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
						filePath.toStdString().c_str());

		setupParameterWidget(false);

		emit textSynthesized();
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
	}

	enableProcessingButtons();
	emit synthesisFinished();
}

// Slot.
void
SynthesisWindow::synthesizeWithManualIntonation()
{
	if (!synthesis_) {
		return;
	}

	emit synthesisStarted();
	disableProcessingButtons();

	try {
		auto& eventList = synthesis_->vtmController->eventList();
		if (eventList.list().empty()) {
			return;
		}
		eventList.clearMacroIntonation();
		eventList.prepareMacroIntonationInterpolation();

		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		audioWorker_->player().fillBuffer([&](std::vector<float>& buffer) {
			synthesis_->vtmController->synthesizeFromEventListToBuffer(
							saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
							buffer);
		});

		setupParameterWidget(false);

		emit playAudioRequested(synthesis_->vtmController->outputSampleRate());
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		enableProcessingButtons();
		emit synthesisFinished();
	}
}

// Slot.
void
SynthesisWindow::synthesizeToFileWithManualIntonation(QString filePath)
{
	if (!synthesis_) {
		return;
	}

	emit synthesisStarted();
	disableProcessingButtons();

	try {
		auto& eventList = synthesis_->vtmController->eventList();
		if (eventList.list().empty()) {
			return;
		}
		eventList.clearMacroIntonation();
		eventList.prepareMacroIntonationInterpolation();

		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		synthesis_->vtmController->synthesizeFromEventListToFile(
						saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
						filePath.toStdString().c_str());

		setupParameterWidget(false);

	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
	}

	enableProcessingButtons();
	emit synthesisFinished();
}

void
SynthesisWindow::on_parameterTableWidget_cellChanged(int row, int column)
{
	bool selected = ui_->parameterTableWidget->item(row, column)->checkState() == Qt::Checked;
	ui_->parameterWidget->changeParameterSelection(row, selected);
}

void
SynthesisWindow::on_xZoomSpinBox_valueChanged(double d)
{
	ui_->parameterWidget->changeXZoom(d);
}

void
SynthesisWindow::on_yZoomSpinBox_valueChanged(double d)
{
	ui_->parameterWidget->changeYZoom(d);
}

// Slot.
void
SynthesisWindow::setupParameterTable()
{
	if (!model_) return;

	auto* table = ui_->parameterTableWidget;
	table->setRowCount(model_->parameterList().size());
	for (unsigned int i = 0; i < model_->parameterList().size(); ++i) {
		auto item = std::make_unique<QTableWidgetItem>(model_->parameterList()[i].name().c_str());
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(Qt::Checked);
		table->setItem(i, 0, item.release());
	}

	ui_->parameterWidget->handleModelUpdate();
}

// Slot.
void
SynthesisWindow::updateMouseTracking(double time, double value)
{
	if (time < -0.1) {
		ui_->timeLineEdit->clear();
		ui_->valueLineEdit->clear();
	} else {
		ui_->timeLineEdit->setText(QString::number(time, 'f', 0));
		ui_->valueLineEdit->setText(QString::number(value, 'f', 2));
	}
}

// Slot.
void
SynthesisWindow::handleAudioError(QString msg)
{
	QMessageBox::critical(this, tr("Error"), msg);

	enableProcessingButtons();
	emit synthesisFinished();
}

// Slot.
void
SynthesisWindow::handleAudioFinished()
{
	if (synthesis_->refVtmController) {
		setSpeechSignal(*synthesis_->refVtmController);
	} else {
		setSpeechSignal(*synthesis_->vtmController);
	}
	ui_->parameterWidget->update();

	enableProcessingButtons();
	emit synthesisFinished();
}

void
SynthesisWindow::resetZoom()
{
	ui_->xZoomSpinBox->setValue(1.0);
	ui_->yZoomSpinBox->setValue(1.0);
}

void
SynthesisWindow::clearSpeechSignal()
{
	speechSignal_.clear();
	speechSamplerate_ = 0.0;
}

void
SynthesisWindow::setSpeechSignal(VTMControlModel::Controller& controller)
{
	audioWorker_->player().copyBuffer(speechSignal_);

	// Adjust the sample rate because the Controller rounds the control period.
	const double controlPeriod = controller.vtmInternalSampleRate() /
					controller.vtmControlModelConfiguration().controlRate;
	const double roundedControlPeriod = std::rint(controlPeriod);
	speechSamplerate_ = controller.outputSampleRate() * (roundedControlPeriod / controlPeriod);
	qDebug("Adjusted speech sample rate: %f", speechSamplerate_);
}

void
SynthesisWindow::setProcessingButtonsEnabled(bool enabled)
{
	ui_->parseButton->setEnabled(enabled);
	ui_->referenceButton->setEnabled(enabled);
	ui_->synthesizeButton->setEnabled(enabled);
	ui_->synthesizeToFileButton->setEnabled(enabled);
}

// Slot.
void
SynthesisWindow::enableProcessingButtons()
{
	setProcessingButtonsEnabled(true);
}

// Slot.
void
SynthesisWindow::disableProcessingButtons()
{
	setProcessingButtonsEnabled(false);
}

void
SynthesisWindow::setupParameterWidget(bool reference)
{
	clearSpeechSignal();
	if (reference) {
		ui_->parameterWidget->updateData(&synthesis_->refVtmController->eventList(), synthesis_->refModel.get(),
							&speechSignal_, &speechSamplerate_);
	} else {
		ui_->parameterWidget->updateData(&synthesis_->vtmController->eventList(), model_,
							&speechSignal_, &speechSamplerate_);
		synthesis_->refVtmController.reset();
		synthesis_->refModel.reset();
	}
	setupParameterTable();
	ui_->parameterWidget->update();
}

} // namespace GS

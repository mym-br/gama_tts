/***************************************************************************
 *  Copyright 2017, 2023 Marcelo Y. Matuda                                 *
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

#include "ParameterModificationWindow.h"

#include <cmath> /* pow */
#include <exception>

#include <QFileDialog>
#include <QMessageBox>
#include <QSignalBlocker>

#include "Controller.h"
#include "Model.h"
#include "ParameterModificationSynthesis.h"
#include "Synthesis.h"
#include "ui_ParameterModificationWindow.h"

#define DEFAULT_AMPLITUDE (10.0)
#define ADD_AMPLITUDE_INCREMENT (0.1)
#define MIN_AMPLITUDE_SPINBOX_VALUE (0.0)
#define MAX_AMPLITUDE_SPINBOX_VALUE (60.0)
#define DEFAULT_OUTPUT_GAIN (0.5)
#define GAIN_INCREMENT (0.01)
#define VTM_PARAM_FILE_NAME "generated__modif_vtm_param.txt"



namespace GS {

ParameterModificationWindow::ParameterModificationWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::ParameterModificationWindow>())
		, model_()
		, synthesis_()
		, prevAmplitude_(DEFAULT_AMPLITUDE)
		, state_(State::stopped)
		, modificationValue_()
		, modificationTimer_(this)
{
	ui_->setupUi(this);

	ui_->amplitudeSpinBox->setSingleStep(ADD_AMPLITUDE_INCREMENT);
	ui_->amplitudeSpinBox->setMinimum(MIN_AMPLITUDE_SPINBOX_VALUE);
	ui_->amplitudeSpinBox->setMaximum(MAX_AMPLITUDE_SPINBOX_VALUE);
	ui_->amplitudeSpinBox->setValue(DEFAULT_AMPLITUDE);

	for (int i = -5; i >= -40; i -= 5) {
		ui_->outputGainComboBox->addItem(QString::number(i), static_cast<double>(i));
	}

	connect(ui_->parameterModificationWidget, &ParameterModificationWidget::modificationStarted,
			this, &ParameterModificationWindow::handleModificationStarted);
	connect(ui_->parameterModificationWidget, &ParameterModificationWidget::offsetChanged,
			this, &ParameterModificationWindow::handleOffsetChanged);

	modificationTimer_.setTimerType(Qt::PreciseTimer);
	connect(&modificationTimer_, &QTimer::timeout,
			this, &ParameterModificationWindow::sendModificationValue);
	connect(&synthesisTimer_   , &QTimer::timeout,
			this, &ParameterModificationWindow::checkSynthesis);

	disableWindow();
}

ParameterModificationWindow::~ParameterModificationWindow()
{
}

void
ParameterModificationWindow::clear()
{
	ui_->parameterComboBox->clear();
	synthesis_ = nullptr;
	model_ = nullptr;
}

void
ParameterModificationWindow::setup(VTMControlModel::Model* model, Synthesis* synthesis)
{
	clearParameterCurveWidget();
	disableWindow();

	if (!model || !synthesis || !synthesis->vtmController) {
		clear();
		return;
	}

	model_ = model;
	synthesis_ = synthesis;

	try {
		synthesis_->paramModifSynth.reset();
		synthesis_->paramModifSynth = std::make_unique<ParameterModificationSynthesis>(
			model_->parameterList().size(),
			synthesis_->vtmController->vtmControlModelConfiguration().controlRate,
			synthesis_->vtmController->vtmConfigData());
	} catch (...) {
		clear();
		throw;
	}

	{
		QSignalBlocker blocker{ui_->parameterComboBox};
		// Fill parameters combobox.
		ui_->parameterComboBox->clear();
		for (unsigned int i = 0, size = model_->parameterList().size(); i < size; ++i) {
			ui_->parameterComboBox->addItem(model_->parameterList()[i].name().c_str(), i);
		}
	}
}

void
ParameterModificationWindow::resetData()
{
	if (!model_) return;

	synthesis_->paramModifSynth->processor().resetData(synthesis_->vtmController->vtmParameterList());

	// Fill the x-axis in the parameter graph.
	modifParamX_.resize(synthesis_->vtmController->vtmParameterList().size());
	const double period = 1.0 / synthesis_->vtmController->vtmControlModelConfiguration().controlRate;
	for (std::size_t i = 0, size = modifParamX_.size(); i < size; ++i) {
		modifParamX_[i] = i * period * 1000.0; // convert to milliseconds
	}

	showModifiedParameterData();

	enableWindow();
}

void
ParameterModificationWindow::enableInput()
{
	setInputEnabled(true);
}

void
ParameterModificationWindow::disableInput()
{
	setInputEnabled(false);
}

void
ParameterModificationWindow::enableWindow()
{
	setEnabled(true);
}

void
ParameterModificationWindow::disableWindow()
{
	setEnabled(false);
}

void
ParameterModificationWindow::on_resetParameterButton_clicked()
{
	if (!model_) return;

	synthesis_->paramModifSynth->processor().resetParameter(ui_->parameterComboBox->currentIndex());
	showModifiedParameterData();
}

void
ParameterModificationWindow::on_synthesizeButton_clicked()
{
	if (!model_) return;

	emit synthesisStarted();
	disableWindow();

	try {
		synthesis_->paramModifSynth->startSynthesis(
			synthesis_->vtmController->outputScale() * outputGain());
	} catch (const std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		enableWindow();
		emit synthesisFinished();
		return;
	}
	synthesisTimer_.start(SYNTH_TIMER_INTERVAL_MS);
}

void
ParameterModificationWindow::on_synthesizeToFileButton_clicked()
{
	if (!model_) return;

	QString filePath = QFileDialog::getSaveFileName(this, tr("Save file"), synthesis_->appConfig.projectDir, tr("WAV files (*.wav)"));
	if (filePath.isEmpty()) {
		return;
	}

	emit synthesisStarted();
	disableWindow();

	try {
		QString vtmParamFilePath;
		bool saveVTMParam = ui_->saveVTMParamCheckBox->isChecked();
		if (saveVTMParam) {
			vtmParamFilePath = synthesis_->appConfig.projectDir + VTM_PARAM_FILE_NAME;
		}

		std::vector<std::vector<float>> vtmParamList;
		synthesis_->paramModifSynth->processor().getModifiedParameterList(vtmParamList);
		synthesis_->vtmController->synthesizeToFile(
					vtmParamList,
					saveVTMParam ? vtmParamFilePath.toStdString().c_str() : nullptr,
					filePath.toStdString().c_str());
	} catch (const Exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
	}

	enableWindow();
	emit synthesisFinished();
}

void
ParameterModificationWindow::on_parameterComboBox_currentIndexChanged(int index)
{
	if (!model_) return;

	if (index >= 0) showModifiedParameterData();
}

void
ParameterModificationWindow::on_addRadioButton_toggled(bool checked)
{
	if (checked) {
		ui_->amplitudeSpinBox->setMinimum(MIN_AMPLITUDE_SPINBOX_VALUE);
		ui_->amplitudeSpinBox->setMaximum(MAX_AMPLITUDE_SPINBOX_VALUE);
		ui_->amplitudeSpinBox->setValue(prevAmplitude_);
	} else {
		prevAmplitude_ = ui_->amplitudeSpinBox->value();
		ui_->amplitudeSpinBox->setMinimum(1.0);
		ui_->amplitudeSpinBox->setMaximum(1.0);
		ui_->amplitudeSpinBox->setValue(1.0);
	}
}

// Slot.
void
ParameterModificationWindow::handleModificationStarted()
{
	if (!model_) return;

	if (state_ == State::stopped) {
		emit synthesisStarted();
		disableInput();
		try {
			synthesis_->paramModifSynth->startSynthesis(
				synthesis_->vtmController->outputScale() * outputGain());
		} catch (const std::exception& exc) {
			QMessageBox::critical(this, tr("Error"), exc.what());
			enableInput();
			emit synthesisFinished();
			return;
		}
		modificationTimer_.start(MODIF_TIMER_INTERVAL_MS);
		state_ = State::running;
	}
}

// Slot.
void
ParameterModificationWindow::handleOffsetChanged(double offset)
{
	if (!model_) return;

	if (state_ == State::stopped) return;

	double modificationValue;
	if (ui_->addRadioButton->isChecked()) {
		modificationValue = offset * ui_->amplitudeSpinBox->value();
	} else {
		modificationValue = 1.0 + offset;
		if (modificationValue < 0.0) {
			modificationValue = 0.0;
		}
	}

	modificationValue_ = modificationValue;
}

// Slot.
void
ParameterModificationWindow::sendModificationValue()
{
	if (!model_) return;

	if (!synthesis_->paramModifSynth->modifyParameter(
				ui_->parameterComboBox->currentIndex(),
				ui_->addRadioButton->isChecked() ?
					ParameterModificationSynthesis::OPER_ADD :
					ParameterModificationSynthesis::OPER_MULTIPLY,
				modificationValue_)) {
		ui_->parameterModificationWidget->stop();
		state_ = State::stopped;
		modificationTimer_.stop();
		qDebug("Modification STOP");

		showModifiedParameterData();

		enableInput();
		emit synthesisFinished();
	}
}

// Slot.
void
ParameterModificationWindow::checkSynthesis()
{
	if (!synthesis_->paramModifSynth->checkSynthesis()) {
		synthesisTimer_.stop();
		qDebug("Synthesis STOP");
		enableWindow();
		emit synthesisFinished();
	}
}

void
ParameterModificationWindow::showModifiedParameterData()
{
	if (!model_ || modifParamX_.empty()) {
		clearParameterCurveWidget();
		return;
	}

	const int parameter = ui_->parameterComboBox->currentIndex();
	synthesis_->paramModifSynth->processor().getParameter(parameter, paramY_);
	synthesis_->paramModifSynth->processor().getModifiedParameter(parameter, modifParamY_);

	if (paramY_.empty() || modifParamY_.empty()) {
		clearParameterCurveWidget();
		return;
	}

	ui_->parameterCurveWidget->updateData(modifParamX_, paramY_);
	ui_->parameterCurveWidget->updateData2(modifParamX_, modifParamY_);
	ui_->parameterCurveWidget->update();
}

void
ParameterModificationWindow::setInputEnabled(bool enabled)
{
	ui_->parameterComboBox->setEnabled(enabled);
	ui_->addRadioButton->setEnabled(enabled);
	ui_->multiplyRadioButton->setEnabled(enabled);
	ui_->amplitudeSpinBox->setEnabled(enabled);
	ui_->outputGainComboBox->setEnabled(enabled);
	//ui_->parameterModificationWidget->setEnabled(enabled);
	ui_->resetParameterButton->setEnabled(enabled);
	ui_->synthesizeButton->setEnabled(enabled);
	ui_->saveVTMParamCheckBox->setEnabled(enabled);
	ui_->synthesizeToFileButton->setEnabled(enabled);
}

double
ParameterModificationWindow::outputGain()
{
	return std::pow(10.0, ui_->outputGainComboBox->currentData().toDouble() * 0.05);
}

void
ParameterModificationWindow::clearParameterCurveWidget()
{
	ui_->parameterCurveWidget->clear();
}

} // namespace GS

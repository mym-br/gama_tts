/***************************************************************************
 *  Copyright 2008, 2014, 2017 Marcelo Y. Matuda                           *
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

#include "InteractiveVTMWindow.h"

#include <cassert>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHash>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "editor_global.h"

#include "AnalysisWindow.h"
#include "Clipboard.h"
#include "ConfigurationData.h"
#include "ParameterLineEdit.h"
#include "ParameterSlider.h"



namespace GS {

/*******************************************************************************
 * Constructor.
 */
InteractiveVTMWindow::InteractiveVTMWindow(const char* configDirPath, bool mainWindow, QWidget* parent)
		: QMainWindow(parent)
		, mainWindow_(mainWindow)
		, configuration_(std::make_unique<InteractiveVTMConfiguration>(configDirPath))
		, transferTimer_()
		, dynamicParamSliderList_(configuration_->dynamicParamNameList.size())
		, dynamicParamEditList_(  configuration_->dynamicParamNameList.size())
		, staticParamSliderList_( configuration_->staticParamNameList.size())
		, staticParamEditList_(   configuration_->staticParamNameList.size())
		, audio_(std::make_unique<InteractiveAudio>(*configuration_))
		, dynamicParameterValueChanged_()
		, analysisWindow_(std::make_unique<AnalysisWindow>())
{
	// Configure the QMainWindow.
	QWidget* widget = new QWidget();
	widget->setMinimumWidth(1300);
	setCentralWidget(widget);

	// What to do if the window is closed.
	connect(qApp, &QApplication::lastWindowClosed, this, &InteractiveVTMWindow::stopAudio);

	// Menu.
	initMenu();

	// Main layout.
	QVBoxLayout* layout = new QVBoxLayout(widget);
	layout->addWidget(initButtons(widget));
	layout->addWidget(initParametersWidget(widget));
	layout->setStretch(1, 1);
	setWindowTitle(INTERACTIVE_NAME);

	// Transfer timer.
	transferTimer_ = new QTimer(this);
	transferTimer_->setTimerType(Qt::PreciseTimer);
	connect(transferTimer_, &QTimer::timeout, this, &InteractiveVTMWindow::transferDynamicParameter);
	transferTimer_->start(TRANSFER_TIMER_TIMEOUT_MS);
}

/*******************************************************************************
 * Destructor.
 */
InteractiveVTMWindow::~InteractiveVTMWindow()
{
}

/*******************************************************************************
 * Initializes the menu.
 */
void
InteractiveVTMWindow::initMenu()
{
	//------------------------------------------------------------
	// Actions.

	QAction* loadDynamicParametersAction = new QAction(tr("Load Dynamic Parameters"), this);
	QAction* saveDynamicParametersAction = new QAction(tr("Save Dynamic Parameters"), this);
	QAction* exitAction{};
	if (mainWindow_) {
		exitAction = new QAction(tr("E&xit"), this);
	}
	QAction* aboutAction = new QAction(tr("About"), this);

	connect(loadDynamicParametersAction, &QAction::triggered, this, &InteractiveVTMWindow::loadDynamicParameters);
	connect(saveDynamicParametersAction, &QAction::triggered, this, &InteractiveVTMWindow::saveDynamicParameters);
	if (mainWindow_) {
		connect(exitAction         , &QAction::triggered, qApp, &QApplication::closeAllWindows);
	}
	connect(aboutAction                , &QAction::triggered, this, &InteractiveVTMWindow::about);

	//------------------------------------------------------------
	// Menu.

	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(loadDynamicParametersAction);
	fileMenu->addAction(saveDynamicParametersAction);
	if (mainWindow_) {
		fileMenu->addSeparator();
		fileMenu->addAction(exitAction);
	}

	QMenu* infoMenu = menuBar()->addMenu(tr("&Info"));
	infoMenu->addAction(aboutAction);
}

/*******************************************************************************
 * Initializes the top buttons.
 */
QWidget*
InteractiveVTMWindow::initButtons(QWidget* parent)
{
	QWidget* widget = new QWidget(parent);

	QPushButton* pasteDynamicParametersButton = new QPushButton(tr("Paste dyn. parameters"), widget);
	QPushButton* copyDynamicParametersButton  = new QPushButton(tr("Copy dyn. parameters"), widget);
	QPushButton* startAudioButton             = new QPushButton(tr("(Re)start"), widget);
	QPushButton* stopAudioButton              = new QPushButton(tr("Stop"), widget);
	QPushButton* reloadButton                 = new QPushButton(tr("Reload Configuration"), widget);
	QPushButton* analysisButton               = new QPushButton(tr("Analysis"), widget);

	QHBoxLayout* layout = new QHBoxLayout(widget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(pasteDynamicParametersButton);
	layout->addWidget(copyDynamicParametersButton);
	layout->addWidget(startAudioButton);
	layout->addWidget(stopAudioButton);
	layout->addWidget(reloadButton);
	layout->addWidget(analysisButton);

	connect(pasteDynamicParametersButton, &QPushButton::clicked, this, &InteractiveVTMWindow::pasteDynamicParameters);
	connect(copyDynamicParametersButton , &QPushButton::clicked, this, &InteractiveVTMWindow::copyDynamicParameters);
	connect(startAudioButton            , &QPushButton::clicked, this, &InteractiveVTMWindow::startAudio);
	connect(stopAudioButton             , &QPushButton::clicked, this, &InteractiveVTMWindow::stopAudio);
	connect(reloadButton                , &QPushButton::clicked, this, &InteractiveVTMWindow::reload);
	connect(analysisButton              , &QPushButton::clicked, this, &InteractiveVTMWindow::showAnalysisWindow);

	return widget;
}

/*******************************************************************************
 * Initializes the parameters widget.
 */
QWidget*
InteractiveVTMWindow::initParametersWidget(QWidget* parent)
{
	QWidget* widget = new QWidget(parent);
	QHBoxLayout* layout = new QHBoxLayout(widget);
	layout->addWidget(initDynamicParametersGroupBox(widget));
	layout->addWidget(initStaticParametersGroupBox(widget));

	return widget;
}

/*******************************************************************************
 * Initializes the dynamic parameter sliders.
 */
QWidget*
InteractiveVTMWindow::initDynamicParametersGroupBox(QWidget* parent)
{
	QGroupBox* group = new QGroupBox(tr("Dynamic Parameters"), parent);
	QVBoxLayout* groupLayout = new QVBoxLayout(group);

	QScrollArea* scrollArea = new QScrollArea(group);
	groupLayout->addWidget(scrollArea);

	QWidget* widget = new QWidget(scrollArea);
	scrollArea->setWidget(widget);
	scrollArea->setWidgetResizable(true); // without this the contents are not shown

	QGridLayout* layout = new QGridLayout(widget);
	layout->setVerticalSpacing(0);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 20);
	layout->setColumnStretch(2, 1);

	const std::size_t numParam = configuration_->dynamicParamNameList.size();
	for (std::size_t i = 0; i < numParam; ++i) {
		// Label.
		layout->addWidget(new QLabel(configuration_->dynamicParamLabelList[i].c_str(), widget), i, 0);

		// Slider.
		dynamicParamSliderList_[i] = new ParameterSlider(
						configuration_->dynamicParamMinList[i],
						configuration_->dynamicParamMaxList[i],
						widget);
		layout->addWidget(dynamicParamSliderList_[i], i, 1);

		// Value.
		dynamicParamEditList_[i] = new ParameterLineEdit(i,
							configuration_->dynamicParamMinList[i],
							configuration_->dynamicParamMaxList[i],
							0.0,
							widget);
		layout->addWidget(dynamicParamEditList_[i], i, 2);

		connect(dynamicParamEditList_[i]  , qOverload<int, float>(&ParameterLineEdit::parameterValueChanged),
			this                      , &InteractiveVTMWindow::setDynamicParameter);
		connect(dynamicParamEditList_[i]  , qOverload<float>(&ParameterLineEdit::parameterValueChanged),
			dynamicParamSliderList_[i], &ParameterSlider::setParameterValue);
		connect(dynamicParamSliderList_[i], &ParameterSlider::parameterValueChanged,
			dynamicParamEditList_[i]  , &ParameterLineEdit::setParameterValueFromSlider);

		dynamicParamEditList_[i]->setParameterValue(configuration_->dynamicParamList[i]);
	}

	// Stretch.
	layout->addWidget(new QWidget(widget), numParam, 0, 1, 3);
	layout->setRowStretch(numParam, 1);

	return group;
}

/*******************************************************************************
 * Initializes the static parameter sliders.
 */
QWidget*
InteractiveVTMWindow::initStaticParametersGroupBox(QWidget* parent)
{
	QGroupBox* group = new QGroupBox(tr("Static Parameters"), parent);
	QVBoxLayout* groupLayout = new QVBoxLayout(group);

	QScrollArea* scrollArea = new QScrollArea(group);
	groupLayout->addWidget(scrollArea);

	QWidget* widget = new QWidget(scrollArea);
	scrollArea->setWidget(widget);
	scrollArea->setWidgetResizable(true); // without this the contents are not shown

	QGridLayout* layout = new QGridLayout(widget);
	layout->setVerticalSpacing(0);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 20);
	layout->setColumnStretch(2, 1);

	const std::size_t numParam = configuration_->staticParamNameList.size();
	for (std::size_t i = 0; i < numParam; ++i) {
		// Label.
		layout->addWidget(new QLabel(configuration_->staticParamLabelList[i].c_str(), widget), i, 0);

		// Slider.
		staticParamSliderList_[i] = new ParameterSlider(
						configuration_->staticParamMinList[i],
						configuration_->staticParamMaxList[i],
						widget);
		layout->addWidget(staticParamSliderList_[i], i, 1);

		// Value.
		staticParamEditList_[i] = new ParameterLineEdit(i,
							configuration_->staticParamMinList[i],
							configuration_->staticParamMaxList[i],
							0.0,
							widget);
		layout->addWidget(staticParamEditList_[i], i, 2);

		connect(staticParamEditList_[i]  , qOverload<int, float>(&ParameterLineEdit::parameterValueChanged),
				this                     , &InteractiveVTMWindow::setStaticParameter);
		connect(staticParamEditList_[i]  , qOverload<float>(&ParameterLineEdit::parameterValueChanged),
				staticParamSliderList_[i], &ParameterSlider::setParameterValue);
		connect(staticParamSliderList_[i], &ParameterSlider::parameterValueChanged,
				staticParamEditList_[i]  , &ParameterLineEdit::setParameterValueFromSlider);

		staticParamEditList_[i]->setParameterValue(configuration_->staticParameter(i));
	}

	// Stretch.
	layout->addWidget(new QWidget(widget), numParam, 0, 1, 3);
	layout->setRowStretch(numParam, 1);

	QPushButton* applyStaticParametersButton = new QPushButton(tr("Appl&y"), group);
	groupLayout->addWidget(applyStaticParametersButton);
	connect(applyStaticParametersButton, &QPushButton::clicked, this, &InteractiveVTMWindow::applyStaticParameters);

	return group;
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::startAudio()
{
	try {
		transferAllDynamicParameters();
		audio_->start();

		analysisWindow_->setData(audio_->sampleRate(), &audio_->analysisRingbuffer(), InteractiveAudio::MAX_NUM_SAMPLES_FOR_ANALYSIS);
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not start audio. Reason: %1").arg(exc.what()));
	}
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::stopAudio()
{
	try {
		analysisWindow_->stop();
		analysisWindow_->setData(0, nullptr, 0);

		audio_->stop();
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not stop audio. Reason: %1").arg(exc.what()));
	}
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::setDynamicParameter(int parameter, float value)
{
	dynamicParameterValue_.index = parameter;
	dynamicParameterValue_.value = value;
	dynamicParameterValueChanged_ = true;
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::setStaticParameter(int parameter, float value)
{
	configuration_->setStaticParameter(parameter, value);
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::transferDynamicParameter()
{
	if (dynamicParameterValueChanged_) {
		JackRingbuffer& rb = audio_->parameterRingbuffer();

		const size_t elementSize = sizeof(VocalTractModelParameterValue);
		if (rb.writeSpace() >= elementSize) {
#ifndef NDEBUG
			size_t bytesWritten =
#endif
			rb.write(reinterpret_cast<const char*>(&dynamicParameterValue_), elementSize);
			assert(bytesWritten == elementSize);
			dynamicParameterValueChanged_ = false;
		}
	}
}

/*******************************************************************************
 *
 */
void
InteractiveVTMWindow::transferAllDynamicParameters()
{
	JackRingbuffer& rb = audio_->parameterRingbuffer();

	const size_t elementSize = sizeof(VocalTractModelParameterValue);
	VocalTractModelParameterValue pv;
	for (std::size_t i = 0, size = configuration_->dynamicParamNameList.size(); i < size; ++i) {
		pv.index = i;
		pv.value = dynamicParamEditList_[i]->parameterValue();
		if (rb.writeSpace() >= elementSize) {
#ifndef NDEBUG
			size_t bytesWritten =
#endif
			rb.write(reinterpret_cast<const char*>(&pv), elementSize);
			assert(bytesWritten == elementSize);
		}
	}
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::loadDynamicParameters()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Parameters:"), currentParametersFileName_, tr("Text files (*.txt)"));
	if (fileName.isEmpty()) {
		return;
	}

	std::vector<float> paramTmp(configuration_->dynamicParamNameList.size());
	try {
		ConfigurationData data(fileName.toStdString());

		for (std::size_t i = 0; i < configuration_->dynamicParamNameList.size(); ++i) {
			paramTmp[i] = data.value<float>(configuration_->dynamicParamNameList[i],
						configuration_->dynamicParamMinList[i],
						configuration_->dynamicParamMaxList[i]);
		}
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not load the dynamic parameters file %1. Reason: %2").arg(fileName, exc.what()));
		return;
	}

	for (std::size_t i = 0, size = configuration_->dynamicParamNameList.size(); i < size; ++i) {
		dynamicParamEditList_[i]->setParameterValue(paramTmp[i]);
	}

	transferAllDynamicParameters();
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::saveDynamicParameters()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Parameters:"), currentParametersFileName_, tr("Text files (*.txt)"));
	if (fileName.isEmpty()) {
		return;
	}

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, tr("Error"), tr("Could not open the file %1.").arg(fileName));
		return;
	}
	currentParametersFileName_ = fileName;

	QTextStream out(&file);
	for (std::size_t i = 0, size = configuration_->dynamicParamNameList.size(); i < size; ++i) {
		out << configuration_->dynamicParamNameList[i].c_str() << " = " << dynamicParamEditList_[i]->parameterString() << '\n';
	}
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::about()
{
	QDialog aboutDialog(this);
	QVBoxLayout* layout = new QVBoxLayout(&aboutDialog);

	QTextEdit* textEdit = new QTextEdit(&aboutDialog);
	textEdit->setReadOnly(true);
	textEdit->setHtml(
		"<pre>"
		INTERACTIVE_NAME " " INTERACTIVE_VERSION "\n\n"

		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n\n"

		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n\n"

		"You should have received a copy of the GNU General Public License\n"
		"along with this program. If not, see http://www.gnu.org/licenses/.\n"
		"</pre>"

		"<hr/>"

		"<pre>"
		"This program uses code from:\n\n"

		"- The Gnuspeech project (http://www.gnu.org/software/gnuspeech/).\n\n"
		"  Provided by David R. Hill, Leonard Manzara and Craig Schock.\n\n"
		"  Gnuspeech is distributed under the terms of the GNU General Public License\n"
		"  as published by the Free Software Foundation, either version 3 of the\n"
		"  License, or (at your option) any later version.\n\n"

		"- Qt (http://www.qt.io/).\n\n"
		"  Provided by Digia Plc.\n\n"
		"  Qt Free Edition is distributed under the terms of the GNU LGPLv2.1 or LGPLv3,\n"
		"  depending on the library (see http://www.qt.io/faq/).\n\n"

		"- FFTW (http://www.fftw.org/).\n\n"
		"  Provided by Matteo Frigo and Massachusetts Institute of Technology.\n\n"
		"  FFTW is free software; you can redistribute it and/or modify it under the\n"
		"  terms of the GNU General Public License as published by the Free Software\n"
		"  Foundation; either version 2 of the License, or (at your option) any\n"
		"  later version.\n\n"

		"- JACK Audio Connection Kit (http://jackaudio.org/).\n\n"
		"  Provided by Paul Davis, Stephane Letz, Jack O'Quinn, Torben Hohn and others.\n\n"
		"  The JACK library is free software; you can redistribute it and/or modify\n"
		"  it under the terms of the GNU Lesser General Public License as published by\n"
		"  the Free Software Foundation; either version 2.1 of the License, or\n"
		"  (at your option) any later version.\n\n"

		"</pre>"
	);
	layout->addWidget(textEdit);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &aboutDialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &aboutDialog, &QDialog::accept);

	aboutDialog.setWindowTitle(tr("About ") + INTERACTIVE_NAME);
	aboutDialog.resize(900, 550);
	aboutDialog.exec();
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::showAnalysisWindow()
{
	analysisWindow_->show();
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::applyStaticParameters()
{
	try {
		audio_->stop();
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not stop audio. Reason: %1").arg(exc.what()));
		return;
	}

	try {
		transferAllDynamicParameters();
		audio_->start();
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not start audio. Reason: %1").arg(exc.what()));
	}
}

/*******************************************************************************
 *
 */
// Slot.
void
InteractiveVTMWindow::reload()
{
	try {
		configuration_->reload();
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not reload the program configuration. Reason: %1").arg(exc.what()));
		return;
	}

	for (std::size_t i = 0, size = dynamicParamSliderList_.size(); i < size; ++i) {
		dynamicParamSliderList_[i]->reset(
					configuration_->dynamicParamMinList[i],
					configuration_->dynamicParamMaxList[i]);
		dynamicParamEditList_[i]->reset(
					configuration_->dynamicParamMinList[i],
					configuration_->dynamicParamMaxList[i],
					configuration_->dynamicParamList[i]);
	}
	for (std::size_t i = 0, size = staticParamSliderList_.size(); i < size; ++i) {
		staticParamSliderList_[i]->reset(
					configuration_->staticParamMinList[i],
					configuration_->staticParamMaxList[i]);
		staticParamEditList_[i]->reset(
					configuration_->staticParamMinList[i],
					configuration_->staticParamMaxList[i],
					configuration_->staticParameter(i));
	}

	try {
		transferAllDynamicParameters();
		audio_->start();

		analysisWindow_->setData(audio_->sampleRate(), &audio_->analysisRingbuffer(), InteractiveAudio::MAX_NUM_SAMPLES_FOR_ANALYSIS);
	} catch (std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), tr("Could not start audio. Reason: %1").arg(exc.what()));
	}
}

void
InteractiveVTMWindow::closeEvent(QCloseEvent* event)
{
	if (mainWindow_) {
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, tr("Quit"), tr("Do you want to quit the application?"),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (ret == QMessageBox::Yes) {
			event->accept();
			qApp->closeAllWindows();
		} else {
			event->ignore();
		}
	} else {
		emit destructionRequested();
		event->accept();
	}
}

void
InteractiveVTMWindow::setDynamicParameters(const QHash<QString, float>& paramMap)
{
	for (std::size_t i = 0, size = configuration_->dynamicParamNameList.size(); i < size; ++i) {
		QString name(configuration_->dynamicParamNameList[i].c_str());
		name.replace(' ', '_');
		auto iter = paramMap.find(name);
		if (iter != paramMap.end()) {
			dynamicParamEditList_[i]->setParameterValue(iter.value());
		}
	}

	transferAllDynamicParameters();
}

void
InteractiveVTMWindow::pasteDynamicParameters()
{
	QHash<QString, float> paramMap = Clipboard::getPostureParameters();
	if (paramMap.empty()) return;

	setDynamicParameters(paramMap);
}

void
InteractiveVTMWindow::copyDynamicParameters()
{
	QHash<QString, float> paramMap;

	for (std::size_t i = 0, size = configuration_->dynamicParamNameList.size(); i < size; ++i) {
		paramMap[configuration_->dynamicParamNameList[i].c_str()] = dynamicParamEditList_[i]->parameterValue();
	}

	Clipboard::putPostureParameters(paramMap);
}

} /* namespace GS */

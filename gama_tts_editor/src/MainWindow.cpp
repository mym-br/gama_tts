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

#include "MainWindow.h"

#include <exception>
#include <iostream>
#include <utility> /* move */

#include <QCloseEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QTextEdit>
#include <QVBoxLayout>

#include "editor_global.h"

#include "DataEntryWindow.h"
#include "Index.h"
#include "InteractiveVTMWindow.h"
#include "IntonationWindow.h"
#include "IntonationParametersWindow.h"
#include "JackConfig.h"
#include "Model.h"
#include "ParameterModificationWindow.h"
#include "PostureEditorWindow.h"
#include "PrototypeManagerWindow.h"
#include "RuleManagerWindow.h"
#include "RuleTesterWindow.h"
#include "Synthesis.h"
#include "SynthesisWindow.h"
#include "TransitionEditorWindow.h"
#include "ui_MainWindow.h"

#define SETTINGS_KEY_DEFAULT_WORK_DIR "default/work_directory"
#define STATUSBAR_TIMEOUT_MS (5000)



namespace GS {

MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent)
		, config_()
		, model_()
		, synthesis_(std::make_unique<Synthesis>(config_))
		, ui_(std::make_unique<Ui::MainWindow>())
		, dataEntryWindow_(std::make_unique<DataEntryWindow>())
		, interactiveVTMWindow_()
		, intonationWindow_(std::make_unique<IntonationWindow>())
		, intonationParametersWindow_(std::make_unique<IntonationParametersWindow>())
		, parameterModificationWindow_(std::make_unique<ParameterModificationWindow>())
		, postureEditorWindow_(std::make_unique<PostureEditorWindow>())
		, prototypeManagerWindow_(std::make_unique<PrototypeManagerWindow>())
		, specialTransitionEditorWindow_(std::make_unique<TransitionEditorWindow>())
		, ruleManagerWindow_(std::make_unique<RuleManagerWindow>())
		, ruleTesterWindow_(std::make_unique<RuleTesterWindow>())
		, synthesisWindow_(std::make_unique<SynthesisWindow>())
		, transitionEditorWindow_(std::make_unique<TransitionEditorWindow>())
{
	ui_->setupUi(this);

	specialTransitionEditorWindow_->setSpecial();

	connect(ui_->quitAction , &QAction::triggered, qApp, &QApplication::closeAllWindows);

	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::editTransitionButtonClicked,
			transitionEditorWindow_.get(), &TransitionEditorWindow::handleEditTransitionButtonClicked);

	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::editSpecialTransitionButtonClicked,
			specialTransitionEditorWindow_.get(), &TransitionEditorWindow::handleEditTransitionButtonClicked);

	connect(dataEntryWindow_.get(), &DataEntryWindow::categoryChanged,
			postureEditorWindow_.get(), &PostureEditorWindow::unselectPosture);
	connect(dataEntryWindow_.get(), &DataEntryWindow::categoryChanged,
			this                      , &MainWindow::updateSynthesis);
	connect(dataEntryWindow_.get(), &DataEntryWindow::parameterChanged,
			postureEditorWindow_.get(), &PostureEditorWindow::unselectPosture);
	connect(dataEntryWindow_.get(), &DataEntryWindow::symbolChanged,
			postureEditorWindow_.get(), &PostureEditorWindow::unselectPosture);

	connect(dataEntryWindow_.get(), &DataEntryWindow::parameterChanged,
			ruleManagerWindow_.get(), &RuleManagerWindow::loadRuleData);
	connect(dataEntryWindow_.get(), &DataEntryWindow::parameterChanged,
			synthesisWindow_.get()  , &SynthesisWindow::setupParameterTable);

	connect(postureEditorWindow_.get(), &PostureEditorWindow::postureChanged,
			ruleManagerWindow_.get(), &RuleManagerWindow::unselectRule);
	connect(postureEditorWindow_.get(), &PostureEditorWindow::postureChanged,
			this                    , &MainWindow::updateSynthesis);
	connect(postureEditorWindow_.get(), &PostureEditorWindow::postureCategoryChanged,
			dataEntryWindow_.get()  , &DataEntryWindow::updateCategoriesTable);

	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			transitionEditorWindow_.get()       , &TransitionEditorWindow::updateEquationsTree);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			transitionEditorWindow_.get()       , &TransitionEditorWindow::updateTransition);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			specialTransitionEditorWindow_.get(), &TransitionEditorWindow::updateEquationsTree);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			specialTransitionEditorWindow_.get(), &TransitionEditorWindow::updateTransition);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			ruleManagerWindow_.get()            , &RuleManagerWindow::setupRuleSymbolEquationsTable);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::equationChanged,
			ruleManagerWindow_.get()            , &RuleManagerWindow::setupEquationsTree);

	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::transitionChanged,
			transitionEditorWindow_.get(), &TransitionEditorWindow::clear);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::transitionChanged,
			ruleManagerWindow_.get()     , &RuleManagerWindow::setupRuleTransitionsTable);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::transitionChanged,
			ruleManagerWindow_.get()     , &RuleManagerWindow::setupTransitionsTree);

	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::specialTransitionChanged,
			specialTransitionEditorWindow_.get(), &TransitionEditorWindow::clear);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::specialTransitionChanged,
			ruleManagerWindow_.get()            , &RuleManagerWindow::setupRuleSpecialTransitionsTable);
	connect(prototypeManagerWindow_.get(), &PrototypeManagerWindow::specialTransitionChanged,
			ruleManagerWindow_.get()            , &RuleManagerWindow::setupSpecialTransitionsTree);

	connect(transitionEditorWindow_.get(), &TransitionEditorWindow::equationReferenceChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::setupEquationsTree);
	connect(transitionEditorWindow_.get(), &TransitionEditorWindow::transitionChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::unselectTransition);

	connect(specialTransitionEditorWindow_.get(), &TransitionEditorWindow::equationReferenceChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::setupEquationsTree);
	connect(specialTransitionEditorWindow_.get(), &TransitionEditorWindow::transitionChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::unselectSpecialTransition);

	connect(ruleManagerWindow_.get(), &RuleManagerWindow::categoryReferenceChanged,
			dataEntryWindow_.get(), &DataEntryWindow::updateCategoriesTable);

	connect(ruleManagerWindow_.get(), &RuleManagerWindow::transitionReferenceChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::setupTransitionsTree);
	connect(ruleManagerWindow_.get(), &RuleManagerWindow::specialTransitionReferenceChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::setupSpecialTransitionsTree);
	connect(ruleManagerWindow_.get(), &RuleManagerWindow::equationReferenceChanged,
			prototypeManagerWindow_.get(), &PrototypeManagerWindow::setupEquationsTree);

	connect(synthesisWindow_.get() , &SynthesisWindow::textSynthesized,
			intonationWindow_.get()           , &IntonationWindow::loadIntonationFromEventList);
	connect(synthesisWindow_.get() , &SynthesisWindow::textSynthesized,
			parameterModificationWindow_.get(), &ParameterModificationWindow::resetData);

	connect(synthesisWindow_.get() , &SynthesisWindow::synthesisStarted,
			intonationWindow_.get()           , &IntonationWindow::disableProcessingButtons);
	connect(synthesisWindow_.get() , &SynthesisWindow::synthesisStarted,
			parameterModificationWindow_.get(), &ParameterModificationWindow::disableWindow);

	connect(synthesisWindow_.get() , &SynthesisWindow::synthesisFinished,
			intonationWindow_.get()           , &IntonationWindow::enableProcessingButtons);
	connect(synthesisWindow_.get() , &SynthesisWindow::synthesisFinished,
			parameterModificationWindow_.get(), &ParameterModificationWindow::enableWindow);

	connect(intonationWindow_.get(), &IntonationWindow::synthesisRequested,
			synthesisWindow_.get() , &SynthesisWindow::synthesizeWithManualIntonation);
	connect(intonationWindow_.get(), &IntonationWindow::synthesisToFileRequested,
			synthesisWindow_.get() , &SynthesisWindow::synthesizeToFileWithManualIntonation);

	connect(parameterModificationWindow_.get(), &ParameterModificationWindow::synthesisStarted,
			synthesisWindow_.get() , &SynthesisWindow::disableProcessingButtons);
	connect(parameterModificationWindow_.get(), &ParameterModificationWindow::synthesisStarted,
			intonationWindow_.get(), &IntonationWindow::disableProcessingButtons);

	connect(parameterModificationWindow_.get(), &ParameterModificationWindow::synthesisFinished,
			synthesisWindow_.get() , &SynthesisWindow::enableProcessingButtons);
	connect(parameterModificationWindow_.get(), &ParameterModificationWindow::synthesisFinished,
			intonationWindow_.get(), &IntonationWindow::enableProcessingButtons);
}

MainWindow::~MainWindow()
{
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
	QMessageBox::StandardButton ret;
	ret = QMessageBox::warning(this, tr("Quit"), tr("Do you want to quit the application?"),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (ret == QMessageBox::Yes) {
		QString dir = config_.projectDir;
		if (!dir.isNull()) {
			QSettings settings;
			settings.setValue(SETTINGS_KEY_DEFAULT_WORK_DIR, dir);
		}

		event->accept();
		qApp->closeAllWindows();
	} else {
		event->ignore();
	}
}

void
MainWindow::on_openAction_triggered()
{
	interactiveVTMWindow_.reset();

	QString dir = config_.projectDir;
	if (dir.isNull()) {
		QSettings settings;
		dir = settings.value(SETTINGS_KEY_DEFAULT_WORK_DIR, QDir::currentPath()).toString();
	}
	QString dirPath = QFileDialog::getExistingDirectory(this, tr("Open directory"), dir,
								 QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dirPath.isEmpty()) {
		return;
	}

	QFile indexFile{dirPath + '/' + VOICE_INDEX_FILE_NAME};
	if (!indexFile.exists()) {
		QMessageBox::critical(this, tr("Error"), tr("Invalid directory (missing index file)."));
		return;
	}

	QDir dirInfo(dirPath);
	config_.projectDir = dirInfo.absolutePath() + '/';

	if (!openModel()) return;

	ui_->dirNameLabel->setText(dirInfo.dirName());
	ui_->statusBar->showMessage(tr("Model opened."), STATUSBAR_TIMEOUT_MS);
}

void
MainWindow::on_saveAction_triggered()
{
	if (!model_) return;

	if (!saveModel()) return;

	ui_->statusBar->showMessage(tr("Model saved."), STATUSBAR_TIMEOUT_MS);
}

#if 0
void
MainWindow::on_saveAsAction_triggered()
{
	if (!model_) return;

	bool done = false;
	do {
		QString filePath = QFileDialog::getSaveFileName(this, tr("Save file"), config_.projectDir + "new_" + config_.dataFileName, tr("XML files (*.xml)"));
		if (filePath.isEmpty()) {
			return;
		}

		QFileInfo fileInfo(filePath);
		QString dir = fileInfo.absolutePath() + '/';
		if (dir != config_.projectDir) {
			QMessageBox::critical(this, tr("Error"), tr("The directory must be the same of the original file."));
			continue;
		}
		config_.dataFileName = fileInfo.fileName();
		ui_->fileNameLabel->setText(fileInfo.fileName());
		done = true;
	} while (!done);

	if (!saveModel()) return;

	ui_->statusBar->showMessage(tr("Model saved."), STATUSBAR_TIMEOUT_MS);
}
#endif

void
MainWindow::on_reloadAction_triggered()
{
	if (!model_) return;

	interactiveVTMWindow_.reset();

	if (!openModel()) return;

	ui_->statusBar->showMessage(tr("Model reloaded."), STATUSBAR_TIMEOUT_MS);
}

void
MainWindow::on_dataEntryButton_clicked()
{
	dataEntryWindow_->show();
	dataEntryWindow_->raise();
}

void
MainWindow::on_ruleManagerButton_clicked()
{
	ruleManagerWindow_->show();
	ruleManagerWindow_->raise();
}

void
MainWindow::on_prototypeManagerButton_clicked()
{
	prototypeManagerWindow_->show();
	prototypeManagerWindow_->raise();
}

void
MainWindow::on_postureEditorButton_clicked()
{
	postureEditorWindow_->show();
	postureEditorWindow_->raise();
}

void
MainWindow::on_intonationWindowButton_clicked()
{
	intonationWindow_->show();
	intonationWindow_->raise();
}

void
MainWindow::on_ruleTesterButton_clicked()
{
	ruleTesterWindow_->show();
	ruleTesterWindow_->raise();
}

void
MainWindow::on_synthesisWindowButton_clicked()
{
	synthesisWindow_->show();
	synthesisWindow_->raise();
}

void
MainWindow::on_intonationParametersButton_clicked()
{
	intonationParametersWindow_->show();
	intonationParametersWindow_->raise();
}

void
MainWindow::on_interactiveVTMButton_clicked()
{
	if (!model_) return;

	if (!interactiveVTMWindow_) {
		interactiveVTMWindow_ = std::make_unique<InteractiveVTMWindow>(config_.projectDir.toStdString().c_str(), false);
		connect(postureEditorWindow_.get(), &PostureEditorWindow::currentPostureChanged,
				interactiveVTMWindow_.get(), &InteractiveVTMWindow::setDynamicParameters);
		connect(interactiveVTMWindow_.get(), &InteractiveVTMWindow::destructionRequested,
				this, &MainWindow::destroyInteractiveVTMWindow, Qt::QueuedConnection);
	}
	interactiveVTMWindow_->show();
	interactiveVTMWindow_->raise();
}

void
MainWindow::on_parameterModificationButton_clicked()
{
	parameterModificationWindow_->show();
	parameterModificationWindow_->raise();
}

bool
MainWindow::openModel()
{
	try {
		Index index{config_.projectDir.toStdString()};
		JackConfig::setupFromFile(index.entry("jack_file").c_str());

		config_.dataFilePath = QString::fromStdString(index.entry("artic_file"));
		model_ = std::make_unique<VTMControlModel::Model>();
		model_->load(config_.dataFilePath.toStdString());

		synthesis_->setup(model_.get());

		dataEntryWindow_->resetModel(model_.get());
		postureEditorWindow_->resetModel(model_.get());
		prototypeManagerWindow_->resetModel(model_.get());
		transitionEditorWindow_->resetModel(model_.get());
		specialTransitionEditorWindow_->resetModel(model_.get());
		ruleManagerWindow_->resetModel(model_.get());
		ruleTesterWindow_->resetModel(model_.get());
		intonationParametersWindow_->setup(synthesis_.get());
		synthesisWindow_->setup(model_.get(), synthesis_.get());
		intonationWindow_->setup(synthesis_.get());
		parameterModificationWindow_->setup(model_.get(), synthesis_.get());

		qDebug() << "### Model opened from" << config_.dataFilePath;
	} catch (const std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());

		parameterModificationWindow_->setup(nullptr, nullptr);
		intonationWindow_->setup(nullptr);
		synthesisWindow_->setup(nullptr, nullptr);
		intonationParametersWindow_->setup(nullptr);
		ruleTesterWindow_->resetModel(nullptr);
		ruleManagerWindow_->resetModel(nullptr);
		specialTransitionEditorWindow_->resetModel(nullptr);
		transitionEditorWindow_->resetModel(nullptr);
		prototypeManagerWindow_->resetModel(nullptr);
		postureEditorWindow_->resetModel(nullptr);
		dataEntryWindow_->resetModel(nullptr);

		synthesis_->setup(nullptr);

		model_.reset();

		return false;
	}
	return true;
}

bool
MainWindow::saveModel()
{
	try {
		model_->validate();

		model_->save(config_.dataFilePath.toStdString());

		qDebug() << "### Model saved to" << config_.dataFilePath;
	} catch (const std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());
		return false;
	}
	return true;
}

void
MainWindow::updateSynthesis()
{
	if (!model_) return;

	try {
		synthesis_->setup(model_.get());

		synthesisWindow_->setup(model_.get(), synthesis_.get());
		intonationWindow_->setup(synthesis_.get());
		intonationParametersWindow_->setup(synthesis_.get());
		parameterModificationWindow_->setup(model_.get(), synthesis_.get());
	} catch (const std::exception& exc) {
		QMessageBox::critical(this, tr("Error"), exc.what());

		parameterModificationWindow_->setup(nullptr, nullptr);
		intonationParametersWindow_->setup(nullptr);
		intonationWindow_->setup(nullptr);
		synthesisWindow_->setup(nullptr, nullptr);
	}
}

// Slot.
void
MainWindow::destroyInteractiveVTMWindow()
{
	interactiveVTMWindow_.reset();
}

void
MainWindow::on_aboutAction_triggered()
{
	QDialog aboutDialog(this);
	QVBoxLayout* layout = new QVBoxLayout(&aboutDialog);

	QTextEdit* textEdit = new QTextEdit(&aboutDialog);
	textEdit->setReadOnly(true);
	textEdit->setHtml(
		"<pre>"
		EDITOR_NAME " " EDITOR_VERSION "\n\n"

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

		"- Gnuspeech (http://www.gnu.org/software/gnuspeech/).\n"
		"  Provided by David R. Hill, Leonard Manzara and Craig Schock.\n"
		"  Gnuspeech is distributed under the terms of the GNU GPL 3\n"
		"  or later version.\n\n"

		"- Qt (http://www.qt.io/).\n"
		"  Provided by Qt Group Plc.\n"
		"  Qt is distributed under the terms of the GNU LGPL 3 or GPL 3,\n"
		"  depending on the module (see http://www.qt.io/faq/).\n\n"

		"- JACK Audio Connection Kit (http://jackaudio.org/).\n"
		"  Provided by Paul Davis, Stephane Letz, Jack O'Quinn, Torben Hohn and others.\n"
		"  JACK is distributed under the terms of the GNU LGPL 2.1\n"
		"  or later version.\n\n"

		"- FFTW (http://fftw.org/).\n"
		"  Provided by Matteo Frigo, Steven G. Johnson and contributors.\n"
		"  FFTW is distributed under the terms of the GNU GPL 2\n"
		"  or later version.\n\n"

		"</pre>"
	);
	layout->addWidget(textEdit);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &aboutDialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &aboutDialog, &QDialog::accept);

	aboutDialog.setWindowTitle(tr("About ") + EDITOR_NAME);
	aboutDialog.resize(900, 550);
	aboutDialog.exec();
}

} // namespace GS

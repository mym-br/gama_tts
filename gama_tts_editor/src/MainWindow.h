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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <memory>

#include <QMainWindow>

#include "AppConfig.h"

namespace Ui {
class MainWindow;
}

namespace GS {

struct Synthesis;

class DataEntryWindow;
class InteractiveVTMWindow;
class IntonationWindow;
class IntonationParametersWindow;
class ParameterModificationWindow;
class PostureEditorWindow;
class PrototypeManagerWindow;
class RuleManagerWindow;
class RuleTesterWindow;
class SynthesisWindow;
class TransitionEditorWindow;

namespace VTMControlModel {
class Model;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent=nullptr);
	virtual ~MainWindow();
protected:
	virtual void closeEvent(QCloseEvent* event);
private slots:
	void on_openAction_triggered();
	void on_saveAction_triggered();
	//void on_saveAsAction_triggered();
	void on_reloadAction_triggered();
	void on_aboutAction_triggered();

	void on_dataEntryButton_clicked();
	void on_ruleManagerButton_clicked();
	void on_prototypeManagerButton_clicked();
	void on_postureEditorButton_clicked();
	void on_intonationWindowButton_clicked();
	void on_ruleTesterButton_clicked();
	void on_synthesisWindowButton_clicked();
	void on_intonationParametersButton_clicked();
	void on_interactiveVTMButton_clicked();
	void on_parameterModificationButton_clicked();

	void updateSynthesis();
	void destroyInteractiveVTMWindow();
private:
	MainWindow(const MainWindow&) = delete;
	MainWindow& operator=(const MainWindow&) = delete;
	MainWindow(MainWindow&&) = delete;
	MainWindow& operator=(MainWindow&&) = delete;

	bool openModel();
	bool saveModel();

	AppConfig config_;
	std::unique_ptr<VTMControlModel::Model> model_;
	std::unique_ptr<Synthesis> synthesis_;

	std::unique_ptr<Ui::MainWindow> ui_;
	std::unique_ptr<DataEntryWindow> dataEntryWindow_;
	std::unique_ptr<InteractiveVTMWindow> interactiveVTMWindow_;
	std::unique_ptr<IntonationWindow> intonationWindow_;
	std::unique_ptr<IntonationParametersWindow> intonationParametersWindow_;
	std::unique_ptr<ParameterModificationWindow> parameterModificationWindow_;
	std::unique_ptr<PostureEditorWindow> postureEditorWindow_;
	std::unique_ptr<PrototypeManagerWindow> prototypeManagerWindow_;
	std::unique_ptr<TransitionEditorWindow> specialTransitionEditorWindow_;
	std::unique_ptr<RuleManagerWindow> ruleManagerWindow_;
	std::unique_ptr<RuleTesterWindow> ruleTesterWindow_;
	std::unique_ptr<SynthesisWindow> synthesisWindow_;
	std::unique_ptr<TransitionEditorWindow> transitionEditorWindow_;
};

} // namespace GS

#endif // MAIN_WINDOW_H

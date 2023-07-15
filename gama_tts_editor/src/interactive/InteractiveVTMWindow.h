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

#ifndef INTERACTIVE_VTM_MAIN_WINDOW_H_
#define INTERACTIVE_VTM_MAIN_WINDOW_H_

#include <memory>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "InteractiveAudio.h"
#include "InteractiveVTMConfiguration.h"
#include "VocalTractModelParameterValue.h"



class QCloseEvent;
class QTimer;
template<typename T, typename U> class QHash;

namespace GS {

class AnalysisWindow;
class ParameterLineEdit;
class ParameterSlider;

class InteractiveVTMWindow : public QMainWindow {
	Q_OBJECT
public:
	InteractiveVTMWindow(const char* configDirPath, bool mainWindow=true, QWidget* parent=nullptr);
	virtual ~InteractiveVTMWindow();
protected:
	virtual void closeEvent(QCloseEvent* event);
public slots:
	void setDynamicParameters(const QHash<QString, float>& paramMap);
private slots:
	void copyDynamicParameters();
	void pasteDynamicParameters();
	void startAudio();
	void stopAudio();
	void setDynamicParameter(int parameter, float value);
	void loadDynamicParameters();
	void saveDynamicParameters();
	void setStaticParameter(int parameter, float value);
	void applyStaticParameters();
	void transferDynamicParameter();
	void reload();
	void about();
	void showAnalysisWindow();
signals:
	void destructionRequested();
private:
	enum {
		TRANSFER_TIMER_TIMEOUT_MS = 25
	};

	InteractiveVTMWindow(const InteractiveVTMWindow&) = delete;
	InteractiveVTMWindow& operator=(const InteractiveVTMWindow&) = delete;
	InteractiveVTMWindow(InteractiveVTMWindow&&) = delete;
	InteractiveVTMWindow& operator=(InteractiveVTMWindow&&) = delete;

	void initMenu();
	QWidget* initButtons(QWidget* parent);
	QWidget* initParametersWidget(QWidget* parent);
	QWidget* initDynamicParametersGroupBox(QWidget* parent);
	QWidget* initStaticParametersGroupBox(QWidget* parent);
	void transferAllDynamicParameters();

	bool mainWindow_;
	std::unique_ptr<InteractiveVTMConfiguration> configuration_;
	QTimer* transferTimer_;
	std::vector<ParameterSlider*>   dynamicParamSliderList_;
	std::vector<ParameterLineEdit*> dynamicParamEditList_;
	std::vector<ParameterSlider*>   staticParamSliderList_;
	std::vector<ParameterLineEdit*> staticParamEditList_;
	std::unique_ptr<InteractiveAudio> audio_;
	QString currentParametersFileName_;
	VocalTractModelParameterValue dynamicParameterValue_;
	bool dynamicParameterValueChanged_;
	std::unique_ptr<AnalysisWindow> analysisWindow_;
};

} /* namespace GS */

#endif /* INTERACTIVE_VTM_MAIN_WINDOW_H_ */

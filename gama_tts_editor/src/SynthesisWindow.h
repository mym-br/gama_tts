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

#ifndef SYNTHESIS_WINDOW_H
#define SYNTHESIS_WINDOW_H

#include <memory>
#include <vector>

#include <QString>
#include <QThread>
#include <QWidget>



namespace Ui {
class SynthesisWindow;
}

namespace GS {

struct Synthesis;
namespace VTMControlModel {
class Controller;
class Model;
}
class AudioWorker;

class SynthesisWindow : public QWidget {
	Q_OBJECT
public:
	explicit SynthesisWindow(QWidget* parent=nullptr);
	virtual ~SynthesisWindow();

	void clear();
	void setup(VTMControlModel::Model* model, Synthesis* synthesis);
signals:
	void textSynthesized();
	void playAudioRequested(double sampleRate);
	void synthesisStarted();
	void synthesisFinished();
public slots:
	void setupParameterTable();
	void synthesizeWithManualIntonation();
	void synthesizeToFileWithManualIntonation(QString filePath);
	void enableProcessingButtons();
	void disableProcessingButtons();
private slots:
	void on_parseButton_clicked();
	void on_referenceButton_clicked();
	void on_synthesizeButton_clicked();
	void on_synthesizeToFileButton_clicked();
	void on_parameterTableWidget_cellChanged(int row, int column);
	void on_xZoomSpinBox_valueChanged(double d);
	void on_yZoomSpinBox_valueChanged(double d);
	void updateMouseTracking(double time, double value);
	void handleAudioError(QString msg);
	void handleAudioFinished();
	void resetZoom();
private:
	SynthesisWindow(const SynthesisWindow&) = delete;
	SynthesisWindow& operator=(const SynthesisWindow&) = delete;
	SynthesisWindow(SynthesisWindow&&) = delete;
	SynthesisWindow& operator=(SynthesisWindow&&) = delete;

	void clearSpeechSignal();
	void setSpeechSignal(VTMControlModel::Controller& controller);
	void setProcessingButtonsEnabled(bool enabled);
	void setupParameterWidget(bool reference=false);

	std::unique_ptr<Ui::SynthesisWindow> ui_;
	VTMControlModel::Model* model_;
	Synthesis* synthesis_;
	QThread audioThread_;
	AudioWorker* audioWorker_;
	std::vector<float> speechSignal_;
	double speechSamplerate_;
};

} // namespace GS

#endif // SYNTHESIS_WINDOW_H

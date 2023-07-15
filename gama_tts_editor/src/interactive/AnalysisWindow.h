/***************************************************************************
 *  Copyright 2017 Marcelo Y. Matuda                                       *
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

#ifndef ANALYSIS_WINDOW_H
#define ANALYSIS_WINDOW_H

#include <memory>
#include <vector>

#include <QVector>
#include <QWidget>

#include <jack/jack.h>



class QTimer;

namespace Ui {
class AnalysisWindow;
}

namespace GS {

class JackRingbuffer;
class SignalDFT;

class AnalysisWindow : public QWidget {
	Q_OBJECT
public:
	explicit AnalysisWindow(QWidget* parent=nullptr);
	virtual ~AnalysisWindow();

	void setData(unsigned int sampleRate, JackRingbuffer* analysisRingbuffer, size_t analysisRingbufferNumSamples);
	void stop();
private slots:
	void on_startStopButton_clicked();
	void showData();
private:
	enum class State {
		stopped,
		enabled
	};

	AnalysisWindow(const AnalysisWindow&) = delete;
	AnalysisWindow& operator=(const AnalysisWindow&) = delete;
	AnalysisWindow(AnalysisWindow&&) = delete;
	AnalysisWindow& operator=(AnalysisWindow&&) = delete;

	void setupWindow();

	std::unique_ptr<Ui::AnalysisWindow> ui_;
	unsigned int sampleRate_;
	JackRingbuffer* analysisRingbuffer_;
	size_t analysisRingbufferNumSamples_;
	QTimer* timer_;
	State state_;
	std::vector<jack_default_audio_sample_t> signal_;
	std::vector<double> plotX_;
	std::vector<double> plotY_;
	std::unique_ptr<SignalDFT> signalDFT_;
	std::vector<double> window_;
};

} /* namespace GS */

#endif // ANALYSIS_WINDOW_H

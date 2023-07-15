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

#include "AnalysisWindow.h"

#include <cassert>
#include <cmath> /* abs, cos, log10, sqrt */
#include <cstddef> /* std::size_t */

#include <QStringList>
#include <QTimer>

#include "JackRingbuffer.h"
#include "SignalDFT.h"
#include "ui_AnalysisWindow.h"

#define TIMER_INTERVAL_MS 500



namespace {

enum WindowType {
	WINDOW_RECTANGULAR,
	WINDOW_TRIANGULAR,
	WINDOW_HANN,
	WINDOW_HAMMING,
	WINDOW_BLACKMAN
};

constexpr unsigned int FREQ_1 = 100;
constexpr unsigned int FREQ_STEP_1 = 100;
constexpr unsigned int FREQ_2 = 1000;
constexpr unsigned int FREQ_STEP_2 = 1000;
constexpr int MAX_DB = -20;
constexpr int MIN_DB = -120;
constexpr int DB_STEP = 10;
constexpr unsigned int MIN_WINDOW_SIZE = 32;
constexpr unsigned int FFT_SIZE = 65536;

} /* namespace */

namespace GS {

AnalysisWindow::AnalysisWindow(QWidget *parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::AnalysisWindow>())
		, sampleRate_()
		, analysisRingbuffer_()
		, analysisRingbufferNumSamples_()
		, timer_(new QTimer(this))
		, state_(State::stopped)
		, signalDFT_(std::make_unique<SignalDFT>(FFT_SIZE))
{
	ui_->setupUi(this);

	connect(timer_, &QTimer::timeout, this, &AnalysisWindow::showData);
	timer_->setInterval(TIMER_INTERVAL_MS);

	QStringList viewComboItems;
	viewComboItems << tr("Spectrum") << tr("Signal");
	ui_->viewComboBox->addItems(viewComboItems);

	QStringList yAxisComboItems;
	yAxisComboItems << tr("Log") << tr("Linear");
	ui_->yAxisComboBox->addItems(yAxisComboItems);

	for (int i = MAX_DB; i >= MIN_DB; i -= DB_STEP) {
		ui_->minDecibelLevelComboBox->addItem(QString::number(i), static_cast<double>(i));
	}
	ui_->minDecibelLevelComboBox->setCurrentIndex(ui_->minDecibelLevelComboBox->count() - 1);

	ui_->windowTypeComboBox->addItem(tr("Rectangular"), WINDOW_RECTANGULAR);
	ui_->windowTypeComboBox->addItem(tr("Triangular") , WINDOW_TRIANGULAR);
	ui_->windowTypeComboBox->addItem(tr("Hann")       , WINDOW_HANN);
	ui_->windowTypeComboBox->addItem(tr("Hamming")    , WINDOW_HAMMING);
	ui_->windowTypeComboBox->addItem(tr("Blackman")   , WINDOW_BLACKMAN);
	ui_->windowTypeComboBox->setCurrentIndex(ui_->windowTypeComboBox->count() - 1);

	ui_->spectrumPlot->setReduceYRange(true);

	connect(ui_->viewComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		ui_->spectrumPlot->setReduceYRange(true);
	});
	connect(ui_->windowSizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		setupWindow();
		ui_->spectrumPlot->setReduceYRange(true);
	});
	connect(ui_->windowTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		setupWindow();
		ui_->spectrumPlot->setReduceYRange(true);
	});
	connect(ui_->maxFreqComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		ui_->spectrumPlot->setReduceYRange(true);
	});
	connect(ui_->yAxisComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		ui_->spectrumPlot->setReduceYRange(true);
	});
	connect(ui_->minDecibelLevelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int /*index*/) {
		ui_->spectrumPlot->setReduceYRange(true);
	});
}

AnalysisWindow::~AnalysisWindow()
{
}

void
AnalysisWindow::setData(unsigned int sampleRate, JackRingbuffer* analysisRingbuffer, size_t analysisRingbufferNumSamples)
{
	if (analysisRingbufferNumSamples > 0 && analysisRingbufferNumSamples != FFT_SIZE) {
		THROW_EXCEPTION(InvalidValueException, "Invalid ring buffer size: " << analysisRingbufferNumSamples <<
				" (should be " << FFT_SIZE << ").");
	}

	sampleRate_ = sampleRate;
	analysisRingbuffer_ = analysisRingbuffer;
	analysisRingbufferNumSamples_ = analysisRingbufferNumSamples;

	ui_->sampleRateLabel->setText(QString::number(sampleRate_));

	signal_.resize(analysisRingbufferNumSamples_);
	plotX_.reserve(analysisRingbufferNumSamples_);
	plotY_.reserve(analysisRingbufferNumSamples_);

	ui_->windowSizeComboBox->clear();
	if (analysisRingbufferNumSamples_ > 0) {
		unsigned int windowSize = MIN_WINDOW_SIZE;
		while (windowSize <= analysisRingbufferNumSamples_) {
			ui_->windowSizeComboBox->addItem(QString::number(windowSize), windowSize);
			windowSize *= 2;
		}
		ui_->windowSizeComboBox->setCurrentIndex(ui_->windowSizeComboBox->count() - 1);
	}

	ui_->maxFreqComboBox->clear();
	if (sampleRate_ > 0) {
		const unsigned int maxFreq = sampleRate_ / 2;
		for (unsigned int f = FREQ_1; f <= maxFreq && f < FREQ_2; f += FREQ_STEP_1) {
			ui_->maxFreqComboBox->addItem(QString::number(f), f);
		}
		unsigned int f = FREQ_2;
		for ( ; f <= maxFreq; f += FREQ_STEP_2) {
			ui_->maxFreqComboBox->addItem(QString::number(f), f);
		}
		if (f < maxFreq + FREQ_STEP_2) {
			ui_->maxFreqComboBox->addItem(QString::number(maxFreq), maxFreq);
		}
		ui_->maxFreqComboBox->setCurrentIndex(ui_->maxFreqComboBox->count() - 1);
	}
}

void
AnalysisWindow::stop()
{
	timer_->stop();
	ui_->startStopButton->setText(tr("Start"));
	state_ = State::stopped;
}

void
AnalysisWindow::on_startStopButton_clicked()
{
	if (state_ == State::stopped) {
		timer_->start();
		ui_->startStopButton->setText(tr("Stop"));
		state_ = State::enabled;
	} else {
		stop();
	}
}

// Slot.
void
AnalysisWindow::showData()
{
	if (sampleRate_ == 0 || !analysisRingbuffer_) {
		stop();
		return;
	}

	assert(!signal_.empty());
	assert(signal_.size() == analysisRingbufferNumSamples_);

	const size_t bufferSize = analysisRingbufferNumSamples_ * sizeof(jack_default_audio_sample_t);
	if (analysisRingbuffer_->readSpace() < bufferSize) {
		return;
	}

	if (ui_->windowSizeComboBox->currentIndex() < 0) {
		return;
	}
	const unsigned int windowSize = ui_->windowSizeComboBox->itemData(ui_->windowSizeComboBox->currentIndex()).toUInt();

	if (ui_->maxFreqComboBox->currentIndex() < 0) {
		return;
	}
	const double maxFreq = ui_->maxFreqComboBox->itemData(ui_->maxFreqComboBox->currentIndex()).toDouble();

	if (ui_->minDecibelLevelComboBox->currentIndex() < 0) {
		return;
	}
	const double minDecibelLevel = ui_->minDecibelLevelComboBox->itemData(ui_->minDecibelLevelComboBox->currentIndex()).toDouble();

	const bool logYAxis = (ui_->yAxisComboBox->currentIndex() == 0);
	const bool spectrumView = (ui_->viewComboBox->currentIndex() == 0);

	// Read data from JACK ringbuffer.
	size_t bytesRead = analysisRingbuffer_->peek(reinterpret_cast<char*>(&signal_[0]), bufferSize);
	assert(bytesRead == bufferSize);
	analysisRingbuffer_->advanceRead(bytesRead);

	// Normalize.
	jack_default_audio_sample_t maxValue = 0.0;
	for (const auto& sample : signal_) {
		const jack_default_audio_sample_t absValue = std::abs(sample);
		if (absValue > maxValue) maxValue = absValue;
	}
	if (maxValue > 0.0) {
		const jack_default_audio_sample_t normCoef = 1.0 / maxValue;
		for (auto& sample : signal_) {
			sample *= normCoef;
		}
	}

	if (spectrumView) {
		assert(signalDFT_);
		assert(window_.size() == windowSize);

		for (unsigned int i = 0; i < windowSize; ++i) {
			signal_[i] *= window_[i];
		}
		for (unsigned int i = windowSize; i < FFT_SIZE; ++i) { // padding
			signal_[i] = 0.0;
		}

		const unsigned int spectrumSize = signalDFT_->outputSize();
		plotX_.resize(spectrumSize);
		plotY_.resize(spectrumSize);

		signalDFT_->execute(&signal_[0], plotY_.data());

		const double freqCoef = static_cast<double>(sampleRate_) / signalDFT_->size();
		const double dftCoef = std::sqrt(1.0 / signalDFT_->size());
		if (logYAxis) {
			for (unsigned int i = 0; i < spectrumSize; ++i) {
				plotX_[i] = i * freqCoef;
				plotY_[i] = 20.0 * std::log10(plotY_[i] * dftCoef);
				if (plotY_[i] < minDecibelLevel) {
					plotY_[i] = minDecibelLevel;
				}
			}
		} else {
			for (unsigned int i = 0; i < spectrumSize; ++i) {
				plotX_[i] = i * freqCoef;
				plotY_[i] *= dftCoef;
			}
		}
	} else {
		plotX_.resize(windowSize);
		plotY_.resize(windowSize);

		for (unsigned int i = 0; i < windowSize; ++i) {
			plotX_[i] = i;
			plotY_[i] = signal_[i];
		}
	}

	if (spectrumView) {
		std::size_t maxSize = plotX_.size();
		for (std::size_t i = 0; i < plotX_.size(); ++i) {
			if (plotX_[i] > maxFreq) {
				maxSize = i;
				break;
			}
		}
		ui_->spectrumPlot->updateData(plotX_, plotY_, maxSize);
	} else {
		ui_->spectrumPlot->updateData(plotX_, plotY_);
	}
	//if (spectrumView && logYAxis) {
	//	ui_->spectrumPlot->graph(0)->valueAxis()->setRange(minDecibelLevel, 0.0);
	//}
	ui_->spectrumPlot->update();
	ui_->spectrumPlot->setReduceYRange(false);
}

void
AnalysisWindow::setupWindow()
{
	if (ui_->windowSizeComboBox->currentIndex() < 0) {
		window_.clear();
		return;
	}
	const unsigned int windowSize = ui_->windowSizeComboBox->itemData(ui_->windowSizeComboBox->currentIndex()).toUInt();
	window_.resize(windowSize);

	const int windowType = ui_->windowTypeComboBox->currentIndex();
	switch (windowType) {
	case WINDOW_RECTANGULAR:
		for (unsigned int i = 0; i < windowSize; ++i) {
			window_[i] = 1.0;
		}
		break;
	case WINDOW_TRIANGULAR:
		{
			const double center = (windowSize - 1) / 2.0;
			for (unsigned int i = 0; i < windowSize; ++i) {
				window_[i] = 1.0 - std::abs((i - center) / center);
			}
		}
		break;
	case WINDOW_HANN:
		{
			const double coef = 2.0 * M_PI / (windowSize - 1);
			for (unsigned int i = 0; i < windowSize; ++i) {
				window_[i] = 0.5 * (1.0 - std::cos(coef * i));
			}
		}
		break;
	case WINDOW_HAMMING:
		{
			const double alpha = 0.53836;
			const double beta = 0.46164;
			const double coef = 2.0 * M_PI / (windowSize - 1);
			for (unsigned int i = 0; i < windowSize; ++i) {
				window_[i] = alpha - beta * std::cos(coef * i);
			}
		}
		break;
	case WINDOW_BLACKMAN:
		{
			const double alpha = 0.16;
			const double a0 = (1.0 - alpha) / 2.0;
			const double a1 = 0.5;
			const double a2 = alpha / 2.0;
			const double coef1 = 2.0 * M_PI / (windowSize - 1);
			const double coef2 = 2.0 * coef1;
			for (unsigned int i = 0; i < windowSize; ++i) {
				window_[i] = a0 - a1 * std::cos(coef1 * i) + a2 * std::cos(coef2 * i);
			}
		}
		break;
	default:
		qDebug("[AnalysisWindow::setupWindow] Invalid window type: %d", windowType);
	}
}

} /* namespace GS */

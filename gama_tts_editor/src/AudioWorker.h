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

#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H

#include <QObject>
#include <QString>

#include "AudioPlayer.h"



namespace GS {

class AudioWorker : public QObject {
	Q_OBJECT
public:
	explicit AudioWorker(QObject* parent=nullptr);
	virtual ~AudioWorker() = default;

	AudioPlayer& player() { return player_; }
signals:
	void finished();
	void errorOccurred(QString);
public slots:
	void playAudio(double sampleRate);
private:
	AudioWorker(const AudioWorker&) = delete;
	AudioWorker& operator=(const AudioWorker&) = delete;
	AudioWorker(AudioWorker&&) = delete;
	AudioWorker& operator=(AudioWorker&&) = delete;

	AudioPlayer player_;
};

} // namespace GS

#endif // AUDIO_WORKER_H

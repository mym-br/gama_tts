
GamaTTS
=======

GamaTTS is an *experimental* articulatory synthesizer that converts text to
speech.

GamaTTS started as a C++ port of the TTS_Server in the original [Gnuspeech][]
system developed for NeXTSTEP, provided by David R. Hill, Leonard Manzara,
Craig Schock and contributors.
The base was the code on Gnuspeech's Subversion repository, revision 672,
downloaded in 2014-08-02. The source code was obtained from the directories:

    nextstep/trunk/ObjectiveC/Monet.realtime
    nextstep/trunk/src/SpeechObject/postMonet/server.monet

Gnuspeech is licensed under the GNU GPLv3 or later.

[Gnuspeech]: http://www.gnu.org/software/gnuspeech/

Status
------

**pre-alpha**

Only english is supported.

The quality of the synthesized speech is much lower than that of the best speech
synthesizers. This software is mostly for people interested in articulatory
speech synthesis.

OS support
----------

Linux+GNU (the software may work on other OSes with POSIX).

External code
-------------

This software includes code from [RapidXml][], provided by Marcin Kalicinski.
See the file src/rapidxml/license.txt for details.

[RapidXml]: http://rapidxml.sourceforge.net/

License
-------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
COPYING file for more details.

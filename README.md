
GamaTTS
=======

GamaTTS is an *experimental* articulatory synthesizer that converts text to
speech, derived from [Gnuspeech][].

[Gnuspeech]: http://www.gnu.org/software/gnuspeech/

Status
------

**alpha**

Only english is supported.

The quality of the synthesized speech is much lower than that of the best speech
synthesizers. This software is mostly for people interested in articulatory
speech synthesis.

Contents
--------

- gama_tts/ - Library and command-line application. This module contains the
  synthesis code.
- gama_tts_editor/ - GamaTTS:Editor, the editor for the articulatory database,
  which is used by gama_tts.
- speechd_module/ - Module for Speech Dispatcher.
- data/ - The articulatory database and other data.
- doc/ - The documentation for GamaTTS and GamaTTS:Editor.

OS support
----------

- GamaTTS: Multiplatform software developed in C++ with no external
  dependencies.
- GamaTTS:Editor: Multiplatform software developed in C++.
  It may work on any OS supported by Qt (5 or 6), FFTW and JACK Audio Connection
  Kit.
- Module for Speech Dispatcher: Multiplatform software developed in C++. It may
  work on any OS supported by Speech Dispatcher and RtAudio.

License
-------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
LICENSE.txt file for more details.

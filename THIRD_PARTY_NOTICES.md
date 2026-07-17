# Third-party notices

StompForge includes NeuralAmpModelerCore v0.5.4 and its pinned AudioDSPTools
and Eigen dependencies. NeuralAmpModelerPlugin v0.7.15 was used as an
integration reference.

NeuralAmpModelerCore
Copyright (c) 2023 Steven Atkinson

NeuralAmpModelerPlugin
Copyright (c) 2022 Steven Atkinson

AudioDSPTools
Copyright (c) 2023 Steven Atkinson

Both projects are distributed under the MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Project sources:

- https://github.com/sdatkinson/NeuralAmpModelerCore
- https://github.com/sdatkinson/NeuralAmpModelerPlugin
- https://github.com/sdatkinson/AudioDSPTools
- https://gitlab.com/libeigen/eigen

Eigen is distributed primarily under the Mozilla Public License 2.0. Its
license files and source are available at the project URL above and in the
pinned NeuralAmpModelerCore dependency fetched by CMake.

JUCE 8.0.10 is dual-licensed under AGPLv3 and the commercial JUCE 8 licence.
StompForge 0.0.1-alpha uses JUCE under the AGPLv3 open-source route:

- https://github.com/juce-framework/JUCE/blob/8.0.10/LICENSE.md
- https://juce.com/legal/juce-8-licence/

JUCE includes third-party components under their respective licences, including
the MIT-licensed VST3 SDK. The authoritative dependency list is in the JUCE
license file linked above.

When StompForge Standalone is built with `ASIO_SDK_PATH`, Steinberg ASIO SDK
code is compiled into the product. That SDK is offered under the proprietary
Steinberg ASIO licence or GPLv3; distribution must comply with the selected
route. ASIO is disabled when `ASIO_SDK_PATH` is not supplied.

AudioDSPTools' Lanczos resampler contains an adapted component whose source
header preserves the iPlug2 and sst-basic-blocks attribution and licence
notices. Those notices remain present in the fetched pinned source.

# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW ON
)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_UTILITY_TOOLS OFF CACHE BOOL "" FORCE)
set(JUCE_INSTALL_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(juce)

# Plugin target
juce_add_plugin(MagentaRetroVST3
    PRODUCT_NAME "MagentaRetroVST3"
    COMPANY_NAME "Google"
    VERSION "0.0.1"
    PLUGIN_MANUFACTURER_CODE GGLG
    PLUGIN_CODE MRV3
    FORMATS VST3
    VST3_CATEGORY "Fx"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(MagentaRetroVST3 PRIVATE
    MagentaRetroVST3Processor.cpp
    MagentaRetroVST3Editor.cpp
    InferenceEngine.cpp
    DspChain.cpp
)

target_include_directories(MagentaRetroVST3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MagentaRetroVST3 PRIVATE
    magentart::core
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_gui_basics
    juce::juce_gui_extra
)

target_compile_definitions(MagentaRetroVST3 PRIVATE
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

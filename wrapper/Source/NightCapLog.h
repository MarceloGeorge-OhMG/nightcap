/*
 * NightCapLog - a small diagnostic log for the NightCap wrapper plug-ins.
 *
 *   macOS: ~/Library/Logs/NightCap/NightCap Plugin.log
 *
 * Every line is written and closed immediately, so when a host crashes (for example while
 * scanning plug-ins) the last line shows how far NightCap got. The first line of each session
 * records the host, plug-in format, process and architecture. Nothing is logged from the audio
 * thread. The file is kept under 1 MB (the previous one is kept as "NightCap Plugin.old.log").
 */
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>

#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "dev"
#endif

namespace NightCapLog
{
    inline juce::File file()
    {
       #if JUCE_MAC
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Logs/NightCap");
       #else
        auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("NightCap/Logs");
       #endif
        return dir.getChildFile ("NightCap Plugin.log");
    }

    inline void writeLine (const juce::String& text)
    {
        static juce::CriticalSection lock;
        const juce::ScopedLock sl (lock);
        auto f = file();
        if (! f.getParentDirectory().createDirectory()) return;
        if (f.getSize() > 1024 * 1024)
            f.moveFileTo (f.getSiblingFile ("NightCap Plugin.old.log"));
        juce::FileOutputStream out (f);
        if (out.openedOk())
        {
            out.writeText (text + "\n", false, false, nullptr);
            out.flush();
        }
    }

    inline juce::String processName()
    {
        // the host application (e.g. "REAPER", "BespokeSynth", or a host's scanner helper process)
        return juce::File::getSpecialLocation (juce::File::hostApplicationPath).getFullPathName();
    }

    /** Session header: written once per process, the first time anything is logged. */
    inline void writeHeaderOnce()
    {
        static std::once_flag once;
        std::call_once (once, []
        {
            juce::PluginHostType host;
            writeLine ("");
            writeLine ("==== " + juce::Time::getCurrentTime().toString (true, true, true, true)
                       + "  " JucePlugin_Name " " JucePlugin_VersionString
                       + "  host: " + juce::String (host.getHostDescription())
                       + "  format: " + juce::AudioProcessor::getWrapperTypeDescription (juce::PluginHostType::getPluginLoadedAs())
                       + "  process: " + processName()
                       + "  arch: " + (juce::SystemStats::getCpuModel().isNotEmpty() ? juce::SystemStats::getCpuModel() : juce::String ("?"))
                       + "  os: " + juce::SystemStats::getOperatingSystemName());
        });
    }

    inline void log (const juce::String& message, const void* instance = nullptr)
    {
        writeHeaderOnce();
        juce::String line = juce::Time::getCurrentTime().formatted ("%H:%M:%S") + "."
                            + juce::String (juce::Time::getCurrentTime().getMilliseconds()).paddedLeft ('0', 3);
        if (instance != nullptr)
            line << "  [" << juce::String::toHexString ((juce::pointer_sized_int) instance).getLastCharacters (6) << "]";
        line << "  " << (juce::MessageManager::existsAndIsCurrentThread() ? "" : "(background thread) ") << message;
        writeLine (line);
    }
}

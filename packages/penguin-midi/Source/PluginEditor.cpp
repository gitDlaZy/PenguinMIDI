#include "PluginEditor.h"

PenguinMIDIEditor::PenguinMIDIEditor(PenguinMIDIProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    auto resourceProvider = [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
    {
        if (url == "/" || url == "/index.html")
        {
            auto* data = reinterpret_cast<const std::byte*>(BinaryData::index_html);
            return juce::WebBrowserComponent::Resource {
                std::vector<std::byte>(data, data + BinaryData::index_htmlSize),
                "text/html"
            };
        }
        return std::nullopt;
    };

    auto options = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                        .getChildFile("PenguinMIDI_WebView"))
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled())
        .withNativeIntegrationEnabled()
        .withResourceProvider(resourceProvider)
        .withNativeFunction("midiNoteOn",
            [this](const juce::Array<juce::var>& args, auto complete)
            {
                if (args.size() >= 4)
                    processor.scheduleMidiNote(
                        (int)args[0], (int)args[1], (int)args[2], (int)args[3]);
                complete(juce::var{});
            })
        .withNativeFunction("getBpm",
            [this](const juce::Array<juce::var>&, auto complete)
            {
                complete(juce::var(processor.getCurrentBpm()));
            })
        .withNativeFunction("savePatterns",
            [this](const juce::Array<juce::var>& args, auto complete)
            {
                if (!args.isEmpty())
                    processor.savePatterns(args[0].toString());
                complete(juce::var{});
            })
        .withNativeFunction("loadPatterns",
            [this](const juce::Array<juce::var>&, auto complete)
            {
                complete(juce::var(processor.loadPatternsJson()));
            });

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable(true, false);
    setSize(1000, 780);

    startTimerHz(2);
}

PenguinMIDIEditor::~PenguinMIDIEditor()
{
    stopTimer();
}

void PenguinMIDIEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void PenguinMIDIEditor::timerCallback()
{
    auto bpm = processor.getCurrentBpm();
    if (std::abs(bpm - lastPushedBpm) > 0.5)
    {
        lastPushedBpm = bpm;
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("bpm", bpm);
        webView->emitEventIfBrowserIsVisible("bpmChanged", juce::var(obj.get()));
    }
}

#include "JuceHeader.h"

#include <algorithm>
using namespace std;

#pragma GCC diagnostic ignored "-Wshadow"

#define PARAM(variable, name, minv, maxv, defv) { \
    this->variable = new AudioParameterFloat(#variable, name, float(minv), float(maxv), float(defv)); \
    addParameter(this->variable); \
}

// for parameter rounding
inline double chop(double d) {
    d = round(d * 10000.0) / 10000.0;
    if (fabs(d) < 0.0000000001) d = 0.0;
    if (fabs(d - round(d)) < 0.0000000001) d = round(d);
    return d;
}

class PluginDSP : public AudioProcessor {
public:
    static constexpr int DATA_FORMAT_SCHEMA = 1;

    PluginDSP()
        // Important: even for a synth, the Input bus should be preserved for the plugin
        // to be able to load and work as AAX
        : AudioProcessor(BusesProperties()
            .withInput("Input", AudioChannelSet::stereo())
            .withOutput("Output", AudioChannelSet::stereo())) {

        PARAM(thresh, "Threshold", -90, 0, 0);
        PARAM(ceiling, "Ceiling", -90, 0, 0);

        PARAM(attack, "Attack", 0.0, 500, 1);
        PARAM(hold, "Hold", 0, 500, 0);
        PARAM(release, "Release", 1, 5000, 75);

        attack->range.setSkewForCentre(30);
        release->range.setSkewForCentre(1000);
        hold->range.setSkewForCentre(75);

        memset(holdv.data(), 0, sizeof(holdv));
    }

    void prepareToPlay(double, int) override {
        env = 0.0;
        holdCounter = 0;
        memset(holdv.data(), 0, sizeof(holdv));
    }

    void releaseResources() override {}

    void processBlock(AudioBuffer<float> &buffer, MidiBuffer &midi) override;

    void processBlock(AudioBuffer<double> &buffer, MidiBuffer &) override;

    bool supportsDoublePrecisionProcessing() const override { return true; }

    AudioProcessorEditor *createEditor() override { return new GenericAudioProcessorEditor(*this); }

    bool hasEditor() const override { return true; }


    const String getName() const override { return "My Audio Plugin"; } // NOLINT(readability-const-return-type)

    bool acceptsMidi() const override { return false; }

    bool producesMidi() const override { return false; }

    double getTailLengthSeconds() const override { return 0; }

    int getNumPrograms() override { return 1; }

    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}

    const String getProgramName(int) override { return "None"; } // NOLINT(readability-const-return-type)

    void changeProgramName(int, const String &) override {}

    void getStateInformation(MemoryBlock &destData) override {
        MemoryOutputStream mo(destData, true);

        auto *obj = new DynamicObject();
        obj->setProperty("schema", DATA_FORMAT_SCHEMA);

        for (const auto *p_: getParameters()) {
            const auto *p = static_cast<const AudioProcessorParameterWithID *>(p_); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            const auto *pf = dynamic_cast<const AudioParameterFloat *>(p);
            const auto *pb = dynamic_cast<const AudioParameterBool *>(p);
            const auto *pi = dynamic_cast<const AudioParameterInt *>(p);
            const auto *pc = dynamic_cast<const AudioParameterChoice *>(p);
            if (pf)
                obj->setProperty(p->paramID, chop(*pf));
            else if (pc)
                obj->setProperty(p->paramID, pc->getIndex());
            else if (pb)
                obj->setProperty(p->paramID, bool(*pb));
            else if (pi)
                obj->setProperty(p->paramID, int(*pi));
            else
                obj->setProperty(p->paramID, p->getValue());
        }

        var json(obj);
        mo.writeText(JSON::toString(json), false, false, "\n");
    }

    void setStateInformation(const void *data, int sizeInBytes) override {
        var json;
        if (JSON::parse(String::fromUTF8((const char *) data, sizeInBytes), json).failed())
            return;

        auto schema = int(json.getProperty("schema", 0));
        if (schema > DATA_FORMAT_SCHEMA) {
            NativeMessageBox::showMessageBoxAsync(
                AlertWindow::AlertIconType::WarningIcon,
                getName(),
                TRANS(
                    "The project was saved with a newer version of this plugin.\n"
                    "The data might not load correctly and might lose elements upon saving\n"));

            // potentially fail to load altogether if the plugin is not compatible with the new format
            //return;
        }

        for (auto *p_: getParameters()) {
            auto *p = static_cast<AudioProcessorParameterWithID *>(p_); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
            auto *pf = dynamic_cast<AudioParameterFloat *>(p);
            auto *pb = dynamic_cast<AudioParameterBool *>(p);
            auto *pi = dynamic_cast<AudioParameterInt *>(p);
            auto *pc = dynamic_cast<AudioParameterChoice *>(p);
            if (pf)
                *pf = json.getProperty(p->paramID, double(*pf));
            else if (pc)
                *pc = json.getProperty(p->paramID, int(pc->getIndex()));
            else if (pb)
                *pb = json.getProperty(p->paramID, bool(*pb));
            else if (pi)
                *pi = json.getProperty(p->paramID, int(*pi));
            else
                p->setValueNotifyingHost(json.getProperty(p->paramID, p->getValue()));
        }
    }

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override {
        const auto &mainInLayout = layouts.getChannelSet(true, 0);
        const auto &mainOutLayout = layouts.getChannelSet(false, 0);

        // requires an output
        if (mainOutLayout.isDisabled()) return false;
 
        // only support mono and stereo
        if (mainOutLayout.size() > 2) return false;

        #if !JucePlugin_IsSynth
            // requires an input
            if (mainInLayout.isDisabled()) return false;

            // support inch = noutch only
            if (mainInLayout != mainOutLayout) return false;
        #endif

        return true;
    }

private:
    static constexpr int RUNNING_MAX_POW = 9; // 2**9 = 512 samples
    AudioBuffer<double> doubleProxy;

    AudioParameterFloat *thresh, *ceiling, *attack, *hold, *release;
    double env = 0.0;
    int holdCounter = 0;

    int bptr = 0;

    array<double,65536> holdv;

    double f1amp = 0.0, f2amp = 0.0, f3amp = 0.0, f4amp = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDSP)
};

void PluginDSP::processBlock(AudioBuffer<double> &buffer, MidiBuffer &) {
    const double th = Decibels::decibelsToGain(double(*thresh));
    const double invThresh = 1.0 / th;
    const double outGain = invThresh * Decibels::decibelsToGain(double(*ceiling));

    const double attackCoeff = 1.0 - pow(0.1, 1000.0 / (*attack * getSampleRate()));
    const double releaseCoeff = 1.0 - pow(0.1, 1000.0 / (*release * getSampleRate()));
    const int holdSamples = int(*hold * getSampleRate() / 1000.0);

    auto *sampL = buffer.getWritePointer(0);
    auto *sampR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : sampL;

    const auto numSamples = buffer.getNumSamples();

    double env = this->env;
    int holdCounter = this->holdCounter;
    constexpr double envlp = 0.1;

    for (int i = 0; i < numSamples; i++) {
        auto amp = fabs(max(sampL[i], sampR[i]) * invThresh);
        amp = max(1.0, amp);

        amp = 1.0 / amp;

        for (int j = 0; j < RUNNING_MAX_POW; j++) {
            int mask = (1 << j) - 1;
            int off = mask + (bptr & mask);
            auto tt = holdv[off]; holdv[off] = amp;
            if (tt < amp) amp = tt;
        }

        // TODO: do not use member vars
        f1amp += (amp - f1amp) * envlp; amp = f1amp;
        f2amp += (amp - f2amp) * envlp; amp = f2amp;
        f3amp += (amp - f3amp) * envlp; amp = f3amp;
        f4amp += (amp - f4amp) * envlp; amp = f4amp;

        bptr++;

        if (env > amp) {
            env += (amp - env) * attackCoeff;
            holdCounter = holdSamples;
        } else {
            if (holdCounter == 0)
                env += (amp - env) * releaseCoeff;
        }

        holdCounter = max(0, holdCounter - 1);

        double sL = sampL[i] * env * outGain;
        double sR = sampR[i] * env * outGain;

        sampL[i] = clamp(sL, -0.9999, 0.9999);
        sampR[i] = clamp(sR, -0.9999, 0.9999);
    }

    this->env = env;
    this->holdCounter = holdCounter;
}

void PluginDSP::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midi) {
    if (doubleProxy.getNumChannels() != buffer.getNumChannels() ||
        doubleProxy.getNumSamples() != buffer.getNumSamples()) {
        doubleProxy = AudioBuffer<double>(buffer.getNumChannels(), buffer.getNumSamples());
    }

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    for (int i = 0; i < numChannels; i++) {
        const auto *r = buffer.getReadPointer(i);
        double *w = doubleProxy.getWritePointer(i);
        for (int j = 0; j < numSamples; j++)
            w[j] = r[j];
    }

    processBlock(doubleProxy, midi);

    for (int i = 0; i < numChannels; i++) {
        auto *w = buffer.getWritePointer(i);
        const double *r = doubleProxy.getReadPointer(i);
        for (int j = 0; j < numSamples; j++)
            w[j] = float(r[j]);
    }
}

AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new PluginDSP();
}


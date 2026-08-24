#include "app/devices/TonePanels.h"

#include "engine/dsp/effects/ToneEffects.h"

namespace incdaw::app {

namespace {

using Eq = engine::dsp::EqEffect;

const DeviceUiSpec& toneSpec()
{
    static const DeviceUiSpec spec = [] {
        using namespace widgets;

        const DeviceUiRange hz{20.0, 20000.0, DeviceSkew::logarithmic, 0.0};

        DeviceUiSpec s;
        s.uid             = "incdaw.tone";
        s.title           = "Tone";
        s.preferredWidth  = 468.0;

        s.root = {
            // The curve is not a second opinion about the filter: the renderer
            // plots "eq-response" from engine::dsp::eqMagnitudeDb, which
            // designs its biquads with the code the audio thread runs.
            drawn(DeviceWidget::drawableCurve,
                  {Eq::lowFreq, Eq::lowGainDb, Eq::midFreq, Eq::midGainDb, Eq::midQ,
                   Eq::highFreq, Eq::highGainDb},
                  "", "eq-response")
                .withTint("accent"),

            grid(3, {knob(Eq::lowGainDb, "BASS").withUnit("dB").asBipolar(),
                     knob(Eq::midGainDb, "MID").withUnit("dB").asBipolar(),
                     knob(Eq::highGainDb, "TREBLE").withUnit("dB").asBipolar()}),

            section("Advanced",
                    {slider(Eq::lowFreq, "Low Freq").withUnit("Hz").withRange(hz),
                     slider(Eq::midFreq, "Mid Freq").withUnit("Hz").withRange(hz),
                     slider(Eq::midQ, "Mid Q"),
                     slider(Eq::highFreq, "High Freq").withUnit("Hz").withRange(hz)})
                .startCollapsed(),
        };

        return s;
    }();

    return spec;
}

} // namespace

void registerTonePanels(std::vector<const DeviceUiSpec*>& specs)
{
    specs.push_back(&toneSpec());
}

} // namespace incdaw::app

#include "app/devices/TonePanels.h"

#include "engine/dsp/effects/ToneEffects.h"

#include <optional>

namespace incdaw::app {

namespace {

using Eq = engine::dsp::EqEffect;

const DeviceUiSpec& toneSpec()
{
    static const DeviceUiSpec spec = [] {
        using namespace widgets;

        const DeviceUiRange hz{20.0, 20000.0, DeviceSkew::logarithmic, 0.0};

        // The curve's vertical axis. ±18 dB shows the whole of the EQ's
        // range without the shelves running off the top.
        constexpr double displayDb = 18.0;

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
                .withTint("accent")
                // The curve is drawn against these two, and the three band
                // handles are placed on them — one source, so a handle can
                // never float off the curve it belongs to.
                .withAxes(hz, {-displayDb, displayDb, DeviceSkew::linear, 0.0})
                // Drag a band by its handle: sideways is its frequency, up
                // and down is its gain, and the wheel over the mid widens or
                // narrows its Q. The shelves have no Q to reach.
                .withPoints({{Eq::lowFreq, Eq::lowGainDb, std::nullopt, "LOW"},
                             {Eq::midFreq, Eq::midGainDb, Eq::midQ, "MID"},
                             {Eq::highFreq, Eq::highGainDb, std::nullopt, "HIGH"}}),

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

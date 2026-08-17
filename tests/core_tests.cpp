#include "core/FrameWindow.hpp"
#include "core/NaNDLExport.hpp"
#include "runtime/AnalysisSession.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <string>

using namespace fwa;

static MacroInput inputAt(std::uint32_t frame, InputAction action = InputAction::Hold) {
    MacroInput input;
    input.frame = frame;
    input.action = action;
    return input;
}

int main() {
    {
        std::map<int, bool> verdicts;
        for (int i = -6; i <= 6; ++i) verdicts[i] = (i >= -2 && i <= 1);
        auto result = FrameWindow::summarize(0, inputAt(100), 6, verdicts);
        assert(result.baselinePassed);
        assert(result.early == 2);
        assert(result.late == 1);
        assert(result.frameWindow == 4);
        assert(result.segments.size() == 1);
        assert(!result.clippedByScanRadius);
    }

    {
        // Recorded on the second valid frame of a four-frame wave gap:
        // valid frames are offsets -1, 0, +1, +2. The analyzer must return N_i=4.
        std::map<int, bool> verdicts;
        for (int i = -4; i <= 4; ++i) verdicts[i] = (i >= -1 && i <= 2);
        auto result = FrameWindow::summarize(0, inputAt(150), 4, verdicts);
        assert(result.baselinePassed);
        assert(result.early == 1);
        assert(result.late == 2);
        assert(result.frameWindow == 4);
        assert(!result.clippedByScanRadius);
    }

    {
        std::map<int, bool> verdicts;
        for (int i = -3; i <= 3; ++i) verdicts[i] = (i == 0);
        auto result = FrameWindow::summarize(1, inputAt(200, InputAction::Release), 3, verdicts);
        assert(result.frameWindow == 1);
        assert(result.early == 0);
        assert(result.late == 0);
    }

    {
        std::map<int, bool> verdicts;
        for (int i = -5; i <= 5; ++i) verdicts[i] = false;
        verdicts[-4] = true;
        verdicts[-3] = true;
        verdicts[-1] = true;
        verdicts[0] = true;
        verdicts[1] = true;
        auto result = FrameWindow::summarize(2, inputAt(300), 5, verdicts);
        assert(result.segments.size() == 2);
        assert(result.frameWindow == 3); // only the island around original frame counts as N_i
        assert(result.early == 1);
        assert(result.late == 1);
    }

    {
        std::map<int, bool> verdicts;
        for (int i = -2; i <= 2; ++i) verdicts[i] = false;
        auto result = FrameWindow::summarize(0, inputAt(1), 2, verdicts);
        assert(!result.baselinePassed);
        assert(result.frameWindow == 0);
    }

    {
        std::map<int, bool> verdicts;
        for (int i = -2; i <= 2; ++i) verdicts[i] = true;
        auto result = FrameWindow::summarize(0, inputAt(123), 2, verdicts);
        assert(result.frameWindow == 5);
        assert(result.clippedByScanRadius);
    }

    {
        Macro macro;
        macro.gameFps = 240;
        macro.windowFps = 240;
        macro.levelName = "Unit Test";
        macro.levelId = 42;

        auto input = inputAt(480);
        std::map<int, bool> verdicts{{-1, true}, {0, true}, {1, true}};
        auto result = FrameWindow::summarize(0, input, 1, verdicts);
        auto json = NaNDLExport::toJson(macro, {result});
        assert(json.find("\"gameFPS\": 240") != std::string::npos);
        assert(json.find("\"windowFPS\": 240") != std::string::npos);
        assert(json.find("\"frameWindow\": 3") != std::string::npos);
        assert(json.find("\"time\": 480.000000") != std::string::npos);
        auto csv = NaNDLExport::toCsv(macro, {result});
        assert(csv.find("Input number,Time (frames),Frame window") != std::string::npos);
    }


    {
        Macro macro;
        macro.inputs.push_back(inputAt(10));
        macro.inputs[0].sequence = 0;

        AnalysisSession session;
        session.start(macro, 2, 5, false);
        assert(session.prepareNextSimulation());
        assert(session.currentJob().offset == 0);
        assert(session.passFrame() == 15);
        session.finishCurrent(true, "baseline");

        assert(session.prepareNextSimulation());
        assert(session.currentJob().offset == -1);
        session.finishCurrent(false, "left fail");

        assert(session.prepareNextSimulation());
        assert(session.currentJob().offset == 1);
        session.finishCurrent(true, "right pass");

        // -2 is skipped in fast mode because -1 already failed.
        assert(session.prepareNextSimulation());
        assert(session.currentJob().offset == 2);
        session.finishCurrent(false, "right fail");

        assert(!session.prepareNextSimulation());
        assert(session.finished());
        assert(session.results().size() == 1);
        assert(session.results()[0].early == 0);
        assert(session.results()[0].late == 1);
        assert(session.results()[0].frameWindow == 2);
    }


    {
        // A HOLD shifted past its paired RELEASE is not a meaningful timing branch.
        Macro macro;
        auto hold = inputAt(10, InputAction::Hold); hold.sequence = 0;
        auto release = inputAt(11, InputAction::Release); release.sequence = 1;
        macro.inputs = {hold, release};

        AnalysisSession session;
        session.start(macro, 2, 0, true);
        // Baseline HOLD.
        assert(session.prepareNextSimulation());
        session.finishCurrent(true, "baseline");
        // offset -1 valid.
        assert(session.prepareNextSimulation());
        session.finishCurrent(true, "early");
        // offset +1 puts HOLD and RELEASE on same frame; sequence still HOLD then RELEASE, valid.
        assert(session.prepareNextSimulation());
        session.finishCurrent(true, "same-frame");
        // offset -2 valid.
        assert(session.prepareNextSimulation());
        session.finishCurrent(true, "early2");
        // offset +2 crosses RELEASE; makeCandidate resolves it internally as FAIL and proceeds to input #2 baseline.
        assert(session.prepareNextSimulation());
        assert(session.currentJob().inputIndex == 1);
        assert(session.currentJob().offset == 0);
    }

    std::cout << "All FWBot frame-window core tests passed.\n";
    return 0;
}

#ifndef COMMANDS_CAMERA_CALIBRATECOMMAND_H
#define COMMANDS_CAMERA_CALIBRATECOMMAND_H

#include "../Command.h"

namespace commands::camera {
    class CalibrateCommand : public Command {
    public:
        CalibrateCommand()
            : Command("camera", "calibrate", "camera calibrate <id> [panSpan] [tiltDown] [tiltUp]",
                      "Measure a VISCA camera's stops (moves it through its full range; defaults 340 90 90)") {}
        void execute(data::Context& ctx, const std::string& args) override;
    };
}

#endif // COMMANDS_CAMERA_CALIBRATECOMMAND_H

#ifndef PRESET_H
#define PRESET_H

namespace cameras {
	class Preset {
	public:
		explicit Preset() : roll(0.0), pan(0.0), pitch(0.0), zoom(1.0) {}
		
		float roll;
		float pan;
		float pitch;
		float zoom;
	};

}

#endif // !PRESET_H

#ifndef CAMERASTATE_H
#define CAMERASTATE_H

namespace cameras {
	class CameraState {
	public:
		explicit CameraState() : roll(0.0), pan(0.0), pitch(0.0), zoom(1.0) {}
		
		float roll;
		float pan;
		float pitch;
		float zoom;
	};

}

#endif // !CAMERASTATE_H

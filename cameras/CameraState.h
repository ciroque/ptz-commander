#ifndef CAMERASTATE_H
#define CAMERASTATE_H

namespace cameras {
	class CameraState {
	public:
		explicit CameraState() : roll(roll), pan(pan), pitch(pitch), zoom(zoom) {}
		
		float roll;
		float pan;
		float pitch;
		float zoom;
	};

}

#endif // !CAMERASTATE_H

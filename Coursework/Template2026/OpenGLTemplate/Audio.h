#pragma once

#ifdef USE_FMOD
#include <fmod.hpp>
#include <fmod_errors.h>
#endif

class CAudio
{
public:
	CAudio();
	~CAudio();
	bool Initialise();
	bool LoadEventSound(const char *filename);
	bool PlayEventSound();
	bool LoadMusicStream(const char *filename);
	bool PlayMusicStream();
	void Update();

private:

#ifdef USE_FMOD
	void FmodErrorCheck(FMOD_RESULT result);
	FMOD_RESULT result;
	FMOD::System *m_FmodSystem;
	FMOD::Sound *m_eventSound;
	FMOD::Sound *m_music;
	FMOD::Channel* m_musicChannel;
#endif
};

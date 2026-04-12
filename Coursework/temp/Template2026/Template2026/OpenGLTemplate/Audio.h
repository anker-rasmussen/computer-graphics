#pragma once

#include <map>
#include <string>

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

	// Legacy single event/music (kept for compatibility)
	bool LoadEventSound(const char *filename);
	bool PlayEventSound();
	bool LoadMusicStream(const char *filename);
	bool PlayMusicStream();

	// Named multi-sound system
	bool LoadSound(const std::string& name, const char* filename, bool looping = false);
	bool PlaySound(const std::string& name, float volume = 1.0f);
	void StopSound(const std::string& name);
	bool IsPlaying(const std::string& name);

	void Update();

private:

#ifdef USE_FMOD
	void FmodErrorCheck(FMOD_RESULT result);
	FMOD_RESULT result;
	FMOD::System *m_FmodSystem;
	FMOD::Sound *m_eventSound;
	FMOD::Sound *m_music;
	FMOD::Channel* m_musicChannel;

	std::map<std::string, FMOD::Sound*> m_sounds;
	std::map<std::string, FMOD::Channel*> m_channels;
#endif
};

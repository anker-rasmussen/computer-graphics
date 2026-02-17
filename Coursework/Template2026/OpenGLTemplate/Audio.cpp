#include "Audio.h"
#include <cstdio>

CAudio::CAudio()
{
#ifdef USE_FMOD
	m_FmodSystem = nullptr;
	m_eventSound = nullptr;
	m_music = nullptr;
	m_musicChannel = nullptr;
#endif
}

CAudio::~CAudio()
{
#ifdef USE_FMOD
	if (m_FmodSystem)
		m_FmodSystem->release();
#endif
}

bool CAudio::Initialise()
{
#ifdef USE_FMOD
	result = FMOD::System_Create(&m_FmodSystem);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	result = m_FmodSystem->init(32, FMOD_INIT_NORMAL, 0);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
#else
	fprintf(stderr, "Audio: FMOD not available, audio disabled\n");
	return true;
#endif
}

// Load an event sound
bool CAudio::LoadEventSound(const char *filename)
{
#ifdef USE_FMOD
	result = m_FmodSystem->createSound(filename, FMOD_DEFAULT, 0, &m_eventSound);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
#else
	(void)filename;
	return true;
#endif
}

// Play an event sound
bool CAudio::PlayEventSound()
{
#ifdef USE_FMOD
	result = m_FmodSystem->playSound(FMOD_CHANNEL_FREE, m_eventSound, false, NULL);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;
	return true;
#else
	return true;
#endif
}


// Load a music stream
bool CAudio::LoadMusicStream(const char *filename)
{
#ifdef USE_FMOD
	result = m_FmodSystem->createStream(filename, FMOD_DEFAULT | FMOD_LOOP_NORMAL, 0, &m_music);
	FmodErrorCheck(result);

	if (result != FMOD_OK)
		return false;

	return true;
#else
	(void)filename;
	return true;
#endif
}

// Play a music stream
bool CAudio::PlayMusicStream()
{
#ifdef USE_FMOD
	result = m_FmodSystem->playSound(FMOD_CHANNEL_FREE, m_music, false, &m_musicChannel);
	FmodErrorCheck(result);

	if (result != FMOD_OK)
		return false;
	return true;
#else
	return true;
#endif
}

#ifdef USE_FMOD
// Check for error
void CAudio::FmodErrorCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		const char *errorString = FMOD_ErrorString(result);
		fprintf(stderr, "FMOD error: %s\n", errorString);
	}
}
#endif

void CAudio::Update()
{
#ifdef USE_FMOD
	m_FmodSystem->update();
#endif
}

// ============================================================================
//
// Audio.cpp
// Copyright Sony Online Entertainment
//
// ============================================================================

#include "clientAudio/FirstClientAudio.h"
#include "clientAudio/Audio.h"

#include "clientAudio/AudioSampleInformation.h"
#include "clientAudio/ConfigClientAudio.h"
#include "clientAudio/Sample2d.h"
#include "clientAudio/Sample3d.h"
#include "clientAudio/SampleStream.h"
#include "clientAudio/SampleCacheEntry.h"
#include "clientAudio/SoundTemplateList.h"
#include "clientAudio/Sound3d.h"
#include "clientAudio/Sound3dTemplate.h"
#include "clientAudio/SoundObject3d.h"
#include "sharedDebug/DataLint.h"
#include "sharedDebug/DebugFlags.h"
#include "sharedDebug/PerformanceTimer.h"
#include "sharedDebug/Profiler.h"
#include "sharedFile/FileNameUtils.h"
#include "sharedFile/Iff.h"
#include "sharedFile/TreeFile.h"
#include "sharedFoundation/ConfigFile.h"
#include "sharedFoundation/LessPointerComparator.h"
#include "sharedFoundation/Os.h"
#include "sharedFoundation/PerThreadData.h"
#include "sharedFoundation/TemporaryCrcString.h"
#include "sharedMath/VectorArgb.h"
#include "sharedObject/Appearance.h"
#include "sharedObject/CellProperty.h"
#include "sharedUtility/DataTable.h"
#include "sharedUtility/LocalMachineOptionManager.h"

#include <limits>
#include <list>
#include <map>

#pragma warning (disable : 4100)

// ============================================================================

namespace AudioNamespace
{
	class ProviderData
	{
	public:

		ProviderData();

		bool        m_supported;
		std::string m_name;
	};

	typedef std::map<int, ProviderData> ProviderMap; //fix

	ProviderMap get3dProviders();

	struct SoundBucketData
	{
		SoundBucketData(Sound2 * const sound, float const distanceSquared, bool const soundIsAlreadyPlaying)
		 : m_sound(sound)
		 , m_distanceSquared(distanceSquared)
		 , m_soundIsAlreadyPlaying(soundIsAlreadyPlaying)
		{
		}

		Sound2 * m_sound;
		float m_distanceSquared;
		bool m_soundIsAlreadyPlaying;

	private:

		//Disabled
		SoundBucketData();
	};

	typedef std::map<CrcString const *, SampleCacheEntry, LessPointerComparator> SampleCache;
	typedef std::map<CrcString const *, int, LessPointerComparator>              MusicOffsetMap;
	typedef std::map<uintptr_t, AbstractFile *>									 FileMap;
	typedef std::map<SampleId, Sample2d>                                         SampleIdToSample2dMap;
	typedef std::map<SampleId, Sample3d>                                         SampleIdToSample3dMap;
	typedef std::map<SampleId, SampleStream>                                     SampleIdToSampleStreamMap;
	typedef std::map<SoundId, Sound2 *>                                          SoundIdToSoundMap;
	typedef std::list<Sound2 *> PrioritizedPlayingSounds;
	typedef std::vector<Sound2 *> UtilitySoundList;
	typedef std::vector<SoundId>                                                 SoundIdList;
	typedef std::list<Sound2 *>                                                  QueuedSamplesToStartList;
	typedef std::map<CrcString const *, SoundBucketData, LessPointerComparator> SoundBucketList;

	PrioritizedPlayingSounds     s_prioritizedPlayingSounds;
	UtilitySoundList             s_utilitySoundList;
	QueuedSamplesToStartList     s_queuedSamplesToStartList;
	ProviderMap                  s_3dProviderMap;
	bool                         s_debugPrintAllocations = false;
	FMOD::Studio::System*		 s_fmod_studio_system = nullptr;
	FMOD::System*				 s_fmod_core_system;
	FMOD_SPEAKERMODE             s_speakers;
	DataTable *                  s_musicDataTable;
	MusicOffsetMap               s_musicOffsetMap;
	SampleCache                  s_sampleCache;                         // The current cached digital sound samples
	SoundIdToSoundMap            s_soundIdToSoundMap;                   // Map of 2d and 3d sounds currently playing
	SampleIdToSample2dMap        s_sampleIdToSample2dMap;
	SampleIdToSample3dMap        s_sampleIdToSample3dMap;
	SampleIdToSampleStreamMap    s_sampleIdToSampleStreamMap;
	FileMap                      s_fileMap;
	SoundObject3d                s_listener;
	SoundIdList                  s_localPurgeList;
	bool                         s_installed = false;
	int                          s_currentCacheSize = 0;
	int                          s_cacheHitCount = 0;
	int                          s_cacheMissCount = 0;
	int                          s_instantRejectionCount = 0;
	int                          s_nextSoundId = 1;
	int                          s_nextSampleId = 1;
	uintptr_t					 s_nextFileHandle = 0;
	float                        s_soundCategoryVolumes[Audio::SC_count];
	float                        s_streamVolume = 1.0f;
	bool                         s_audioEnabled = true;
	float                        s_masterVolume = 1.0f;
	int                          s_requestedMaxNumberOfSamples = 32;
	int                          s_maxCached2dSampleSize = 0;
	ConstWatcher<Object>         s_listenerObject;
	float const                  s_rollOffFactor = 0.0f; // The volume of sounds is based on linear distance
	float                        s_occlusion = 0.0f;
	float                        s_obstruction = 0.0f;
	std::string                  s_soundProvider;
	bool                         s_debugWindow = false;
	bool                         s_debugWindowPrioritizedSounds = false;
	bool                         s_debugWindowQueuedSounds = false;
	bool                         s_debugWindowPlayingSamples = false;
	bool                         s_debugSoundStartStop = false;
	bool                         s_debugDumpCachedSamplesTextFile = false;
	bool                         s_debugVisuals = false;
	bool                         s_debugTimerDelay = false;
	bool                         s_toolApplication = false;
	int                          s_priorityCount = 10;
	int                          s_timerHighestDelay = 0;
	int                          s_timerCurrentDelay = 0;
	int                          s_digitalLatency = 0;
	int                          s_digitalCpuPercent = 0;
	int                          s_nonVoiceoverFadeCount = 0;
	int                          s_nonBackgroundFadeCount = 0;
	int                          s_allAudioFadeCount = 0;
	int const                    s_bufferFragmentsMin = 16;
	float                        s_averageTimerDelay = 0.0f;
	PerformanceTimer *           s_audioServePerformanceTimer = nullptr;
	float                        s_globalAudioFadeVolume = 1.0f;
	float                        s_allAudioFadeFactor = 0.5f;
	float                        s_nonBuffereMusicFadeVolume = 1.0f;
	QueuedSamplesToStartList s_centerBucket;
	SoundBucketList s_leftBucket;
	SoundBucketList s_rightBucket;
	SoundBucketList s_frontBucket;
	SoundBucketList s_backBucket;
	bool s_consolidateQueuedSounds = true;
	int s_soundFallOffPower = 3;
	float const s_centerConsolidateDistanceSquared = sqr(40.0f);
	int s_maxDigitalMixerChannels = 0;
	int s_allocated2dSampleHandles = 0;
	int s_allocated3dSampleHandles = 0;
	bool s_disableMiles = false;

	bool s_silenceNonBufferedMusic = false;

	int         getMaxStreamSampleCount();
	char const *getAttenuationMethodString(Audio::AttenuationMethod const attenuationMethod);

	CrcString const *  cacheSound(TemporaryCrcString const &path, SampleCache::iterator &iterSampleCache, bool const needToCacheSample);
	void               cacheSample(TemporaryCrcString const &path, AbstractFile *file, SampleCache::iterator &iterSampleCache);
	void               clearMusicOffsets();
	bool               rejectSound(SoundTemplate const &soundTemplate, Vector const &position);
	void               decreaseReferenceCount(CrcString const &path);
	void               addPlayingSound(Sound2 const &sound);
	int                getPlayingSoundCount(CrcString const *soundPath);
	SoundId            playSound(char const * const soundPath, Vector const * const position, Object const * const object, char const *hardPointName, CellProperty const * const parentCell);
	SampleId           createSampleId(Sound2 &sound);
	void               stopSound(SoundId const &soundId, float const fadeOutTime, bool const keepAlive);
	void               removeSoundFromPrioritizedPlayingSounds(Sound2 const &sound);
	void               setSoundCategoryVolume(Audio::SoundCategory const soundCategory, float const volume);
	float              getSoundCategoryVolume(Audio::SoundCategory const soundCategory, bool settingOnly=false);
	bool			   queueSample(SoundBucketList & soundBucketList, Sound2 & sound, float const distanceSquared, bool const soundIsAlreadyPlaying);
	void			   insertionSort(QueuedSamplesToStartList & list, Sound2 & sound);
	int				   getMaxNumberOfSamples();
	FMOD_SPEAKERMODE   getProviderSpec(std::string const & provider);
	std::string const  getSpeakerSpec();
	bool			   isNonBufferedMusic(Audio::SoundCategory const soundCategory);

#ifdef _DEBUG
	char const * const getSoundCategoryString(Audio::SoundCategory const soundCategory);
#endif // _DEBUG

#ifdef _DEBUG
	typedef std::map<uintptr_t, std::string> HandleNameMap;
	HandleNameMap ms_handleNameMap;

	typedef std::set<uintptr_t> HandleSet;
	HandleSet ms_fileCloseHandleSet;

	void determineCallbackError(char const * const callbackName, uintptr_t const handle)
	{
		HandleNameMap::iterator iter = ms_handleNameMap.find(handle);
		bool const fileOpen = iter != ms_handleNameMap.end();
		bool const fileClosed = ms_fileCloseHandleSet.find(handle) != ms_fileCloseHandleSet.end();

		DEBUG_FATAL(true, ("Trying to file %s on a file that no longer exists. handle=%d, filename=%s, open=%d, still open=%d, closed=%d",
			callbackName,
			handle,
			fileOpen ? iter->second.c_str() : "invalid handle",
			fileOpen ? 1 : 0,
			s_fileMap.find(handle) != s_fileMap.end() ? 1 : 0,
			fileClosed ? 1 : 0));
	}
#endif
}

using namespace AudioNamespace;

// Callbacks for Miles to the TreeFile system
static FMOD_RESULT F_CALLBACK fileOpenCallBack(const char *fileName, unsigned int *fileSize, void **fileHandle, void *userData);
static FMOD_RESULT F_CALLBACK fileCloseCallBack(void *fileHandle, void *userData);
static FMOD_RESULT F_CALLBACK fileSeekCallBack(void *fileHandle, unsigned int offset, void *userData);
static FMOD_RESULT F_CALLBACK fileReadCallBack(void *fileHandle, void *buffer, unsigned int sizeBytes, unsigned int *bytesRead, void *userData);

static SoundId attachSound(SoundTemplate const *soundTemplate, Object const *object, char const *hardPointName=0);
static bool cacheSound(SoundTemplate const *soundTemplate);
static int getFrequency();
static SampleIdToSample3dMap::iterator getIterSampleIdToSample3dMap(SampleId const &sampleId);
static SoundIdToSoundMap::iterator getIterSoundIdToSoundMap(SoundId const &soundId);
static void getSampleTime(char const *path, byte *fileImage, int fileSize, float &timeTotal, float &timeCurrent);
static bool isCached(CrcString const &path, SampleCache::iterator &iterSampleCache);
static bool isSample2d(SampleId const &sampleId, SampleIdToSample2dMap::iterator &iterSampleIdToSample2dMap);
static bool isSample3d(SampleId const &sampleId, SampleIdToSample3dMap::iterator &iterSampleIdToSample3dMap);
static bool isSampleStream(SampleId const &sampleId, SampleIdToSampleStreamMap::iterator &iterSampleIdToSampleStreamMap);
static SoundId playSound2d(SoundTemplate const *soundTemplate, Vector const * const position, CellProperty const * const parentCell);
static SoundId playSound3d(SoundTemplate const *soundTemplate, Vector const &position, CellProperty const * const parentCell);
static int getNextSampleId();
static int getNextSoundId();
static Audio::AttenuationMethod getAttenuationMethod(SoundId const &soundId);
static char const * getAttenuationMethodString(Audio::AttenuationMethod const attenuationMethod);
static void stopAllSounds(float const fadeOutTime, bool const suspendActiveSounds);
static void stopSample(Sound2 const &sound);
static FMOD_RESULT F_CALLBACK endOfSample2dCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2);
static FMOD_RESULT F_CALLBACK endOfSample3dCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2);
static FMOD_RESULT F_CALLBACK endOfSampleStreamCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2);

// ============================================================================
//
// ProviderData
//
// ============================================================================

//-----------------------------------------------------------------------------
ProviderData::ProviderData()
 : m_supported(false)
 , m_name("")
{
}

// ============================================================================
//
// AudioNamespace
//
// ============================================================================

//-----------------------------------------------------------------------------

char const *AudioNamespace::getAttenuationMethodString(Audio::AttenuationMethod const attenuationMethod)
{
	char const *result = "invalid";

	switch (attenuationMethod)
	{
		case Audio::AM_none: { result = "attenuation: none"; } break;
		case Audio::AM_2d: { result = "attenuation: 2D"; } break;
		case Audio::AM_3d: { result = "attenuation: 3D"; } break;
	}

	return result;
}

//-----------------------------------------------------------------------------

void AudioNamespace::clearMusicOffsets()
{
	for(auto& iterMusicOffsetMap : s_musicOffsetMap)
	{
		delete iterMusicOffsetMap.first;
	}

	s_musicOffsetMap.clear();
}

//-----------------------------------------------------------------------------

bool AudioNamespace::rejectSound(SoundTemplate const &soundTemplate, Vector const &position)
{
	bool result = false;

	if (s_listenerObject != nullptr && !soundTemplate.isInfiniteLooping())
	{
		float const maxAudibleDistance = Audio::getFallOffDistance(soundTemplate.getDistanceAtMaxVolume());
		float const distanceSquaredFromSound = Vector(position - s_listenerObject->getPosition_w()).magnitudeSquared();

		if (distanceSquaredFromSound > (maxAudibleDistance * maxAudibleDistance))
		{
			result = true;
			//DEBUG_REPORT_LOG(true, ("Audio: rejected %s\n", soundTemplate.getName()));
			++s_instantRejectionCount;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void AudioNamespace::decreaseReferenceCount(CrcString const &path)
{
	auto iterSampleCache = s_sampleCache.find(&path);

	if (iterSampleCache != s_sampleCache.end())
	{
		// Decrement the reference count
		--iterSampleCache->second.m_referenceCount;

		//DEBUG_REPORT_LOG(Audio::isDebugEnabled(), ("Audio: Ref count decreased: %d %s\n", iterSampleCache->second.m_referenceCount, iterSampleCache->first->getString()));
		
		if(iterSampleCache->second.m_referenceCount <= 0)
		{
			// Deallocate its memory
			delete [] iterSampleCache->second.m_sampleRawData;
			s_currentCacheSize -= iterSampleCache->second.m_fileSize;

			DEBUG_REPORT_LOG(Audio::isDebugEnabled(), ("Audio: Sample removed from cache: %s - cache size (%5dk)\n", iterSampleCache->first->getString(), Audio::getCurrentCacheSize() / 1024));

			// Remove it from the sound cache
			delete iterSampleCache->first;

			s_sampleCache.erase(iterSampleCache);
		}
	}
	else
	{
		DEBUG_REPORT_LOG(true, ("Audio: Trying to remove a sample that is not in the cache: %s\n", path.getString()));
	}
}

//-----------------------------------------------------------------------------

void AudioNamespace::addPlayingSound(Sound2 const &sound)
{
	NP_PROFILER_AUTO_BLOCK_DEFINE("AudioNamespace::addPlayingSound");

	SoundIdToSoundMap::const_iterator iterSoundIdToSoundMap = s_soundIdToSoundMap.find(sound.getSoundId());

	if(iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		auto iterPrioritizedPlayingSounds = s_prioritizedPlayingSounds.begin();
		
		for(; iterPrioritizedPlayingSounds != s_prioritizedPlayingSounds.end(); ++iterPrioritizedPlayingSounds)
		{
			int const newPriority = iterSoundIdToSoundMap->second->getTemplate()->getPriority();
			int const currentPriority = (*iterPrioritizedPlayingSounds)->getTemplate()->getPriority();
			float const newDistanceSquared = iterSoundIdToSoundMap->second->getDistanceSquaredFromListener();
			float const currentDistanceSquared = (*iterPrioritizedPlayingSounds)->getDistanceSquaredFromListener();

			if(newPriority < currentPriority || (newPriority == currentPriority && newDistanceSquared < currentDistanceSquared))
			{
				s_prioritizedPlayingSounds.insert(iterPrioritizedPlayingSounds, NON_NULL(iterSoundIdToSoundMap->second));
				//DEBUG_REPORT_LOG(s_debugSoundStartStop, ("1 Prioritized sound added: %d %s\n", sound.getSoundId().getId(), iterSoundIdToSoundMap->second->getTemplate()->getName()));
				break;
			}
		}

		if(iterPrioritizedPlayingSounds == s_prioritizedPlayingSounds.end())
		{
			s_prioritizedPlayingSounds.push_back(NON_NULL(iterSoundIdToSoundMap->second));
			//DEBUG_REPORT_LOG(s_debugSoundStartStop, ("2 Prioritized sound added: %d %s\n", sound.getSoundId().getId(), iterSoundIdToSoundMap->second->getTemplate()->getName()));
		}
	}
	else
	{
		DEBUG_FATAL(true, ("This sound should exist: %s", sound.getTemplate()->getName()));
	}

	DEBUG_WARNING((static_cast<int>(s_prioritizedPlayingSounds.size()) > getMaxNumberOfSamples()), ("Too many sounds are playing: %d/ %d max", static_cast<int>(s_prioritizedPlayingSounds.size()), getMaxNumberOfSamples()));
}

//-----------------------------------------------------------------------------
int AudioNamespace::getPlayingSoundCount(CrcString const *soundPath)
{
	int result = 0;

	if (soundPath != nullptr)
	{
		for(auto* sound : s_prioritizedPlayingSounds)
		{
			if (sound->getTemplate()->getCrcName() == *soundPath)
			{
				++result;
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId AudioNamespace::playSound(char const * const soundPath, Vector const * const position, Object const * const object, char const *hardPointName, CellProperty const * const parentCell)
{
//	DEBUG_WARNING((object != NULL) && !object->isInWorld(), ("Trying to play a sound on an object that is not in the world: %s object: %s", soundPath, object->getObjectTemplateName()));

	SoundId result (0, soundPath);

	if (s_installed && soundPath != nullptr)
	{
		// Get the extension
		char const *extension = soundPath;
		char const *currentPosition = soundPath;
		bool done = false;

		do
		{
			currentPosition = strchr(extension, '.');

			if (currentPosition != nullptr)
			{
				extension = currentPosition + 1;
			}
			else
			{
				done = true;
			}
		}
		while (!done);

		// Make sure this is a supported sound file
		if (_stricmp(extension, "snd") == 0)
		{
			SoundTemplate const *soundTemplate = SoundTemplateList::fetch(soundPath);

			if (soundTemplate != nullptr)
			{
				// Don't reject a stereo sound
				if (soundTemplate->getAttenuationMethod() != Audio::AM_none)
				{
					// The sound looks valid, queue it
					if (position != nullptr)
					{
						if (rejectSound(*soundTemplate, *position))
						{
							// Sound is out of range
							//-- Release local resources.
							SoundTemplateList::release(soundTemplate);
							return result;
						}
					}

					if (object != nullptr)
					{
						if (rejectSound(*soundTemplate, object->getPosition_w()))
						{
							// Sound is out of range
							//-- Release local resources.
							SoundTemplateList::release(soundTemplate);
							return result;
						}
					}
				}

				if (soundTemplate->is3d())
				{
					if (object != nullptr)
					{
						result = attachSound(soundTemplate, object, hardPointName);
					}
					else if (position != nullptr)
					{
						result = playSound3d(soundTemplate, *position, parentCell);
					}
					else
					{
						DEBUG_WARNING(true, ("Playing a 3d sound template as a 2d sound because neither a position or object was specified: %s", soundPath));

						result = playSound2d(soundTemplate, nullptr, parentCell);
					}
				}
				else if (soundTemplate->is2d())
				{
					if (object != nullptr && soundTemplate->getAttenuationMethod() != Audio::AM_none)
					{
						result = attachSound(soundTemplate, object, hardPointName);
					}
					else
					{
						result = playSound2d(soundTemplate, position, parentCell);
					}
				}
				else
				{
					DEBUG_WARNING(true, ("Attempting to play a non 2d or 3d sound template: %s", soundPath));
				}

				SoundTemplateList::release(soundTemplate);
			}
			else
			{
				if (Audio::isEnabled())
				{
					DEBUG_WARNING(true, ("SoundTemplateList returned a NULL sound template: %s", soundPath));
				}
			}
		}
		else
		{
			WARNING(true, ("Attempting to play unsupported file: %s", soundPath));
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

SampleId AudioNamespace::createSampleId(Sound2 &sound)
{
	SampleId sampleId;

	if (sound.getSamplePath() == nullptr)
	{
		DEBUG_WARNING(true, ("Trying to play a sound with a NULL sample path: %s", sound.getTemplate()->getName()));
	}
	else
	{
		// Get the size of the sample
		int sampleSize = 0;

		SampleCache::const_iterator iterSampleCache = s_sampleCache.find(sound.getSamplePath());

		if(iterSampleCache != s_sampleCache.end() && iterSampleCache->second.m_fileSize != 0)
		{
			sampleSize = iterSampleCache->second.m_fileSize;
		}
		else
		{
			sampleSize = Audio::getSampleSize(sound.getSamplePath()->getString());
		}

		if(sound.is2d())
		{
			// See if this sample needs to be streamed or cached
			if(sampleSize > Audio::getMaxCached2dSampleSize())
			{
				// Stream the sample
				SampleStream sampleStream;

				FMOD_MODE eMode = FMOD_DEFAULT;
			    eMode |= FMOD_2D;
			    eMode |= FMOD_LOOP_OFF;
			    eMode |= FMOD_CREATESTREAM;

				s_fmod_core_system->createSound(sound.getSamplePath()->getString(), eMode, nullptr, &sampleStream.mFmodStream);

				if (sampleStream.mFmodStream != nullptr)
				{
					sampleStream.setPath(sound.getSamplePath()->getString());
					sampleStream.m_sound = &sound;
					sampleStream.m_status = Audio::PS_notStarted;

					// Save the current id
					sampleId = SampleId(getNextSampleId());

					// Add it to the stream sample map
					s_sampleIdToSampleStreamMap.insert(std::make_pair(sampleId, sampleStream));
				}
			}
			else if(sampleSize > 0)
			{
				// Save the id to path mapping
				Sample2d sample2d;

				// Create the sample handle
				FMOD_MODE eMode = FMOD_DEFAULT;
			    eMode |= FMOD_2D;
			    eMode |= FMOD_LOOP_OFF;
			    eMode |= FMOD_CREATECOMPRESSEDSAMPLE;

				s_fmod_core_system->createSound(sound.getSamplePath()->getString(), eMode, nullptr, &sample2d.mFmodSample);

				if (sample2d.mFmodSample != nullptr)
				{
					// Clear out the sample
					++s_allocated2dSampleHandles;

					sample2d.setPath(sound.getSamplePath()->getString());
					sample2d.m_sound = &sound;
					sample2d.m_status = Audio::PS_notStarted;

					// Save the current id
					sampleId = SampleId(getNextSampleId());

					// Add it to the 2d sample map
					s_sampleIdToSample2dMap.insert(std::make_pair(sampleId, sample2d));
				}
			}
		}
		else if(sound.is3d())
		{
			// Save the id to path mapping
			Sample3d sample3d;

			// Create the sample handle
			FMOD_MODE eMode = FMOD_DEFAULT;
		    eMode |= FMOD_3D;
		    eMode |= FMOD_LOOP_OFF;
		    eMode |= FMOD_CREATECOMPRESSEDSAMPLE;

			s_fmod_core_system->createSound(sound.getSamplePath()->getString(), eMode, nullptr, &sample3d.mFmodSample);

			if (sample3d.mFmodSample != nullptr)
			{
				++s_allocated3dSampleHandles;

				sample3d.setPath(sound.getSamplePath()->getString(), sampleSize);
				sample3d.m_sound = &sound;
				sample3d.m_status = Audio::PS_notStarted;

				// Save the current id
				sampleId = SampleId(getNextSampleId());

				// Add it to the map
				s_sampleIdToSample3dMap.insert(std::make_pair(sampleId, sample3d));
			}
		}
		else
		{
			DEBUG_FATAL(true, ("Sound is neither 2d or 3d"));
		}
	}

	return sampleId;
}

//-----------------------------------------------------------------------------

void AudioNamespace::stopSound(SoundId const &soundId, float const fadeOutTime, bool const keepAlive)
{
	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		// Stop the sound
		Sound2 * const sound = NON_NULL(iterSoundIdToSoundMap->second);
		sound->stop(fadeOutTime, keepAlive);

		if (!keepAlive)
		{
			if (fadeOutTime <= 0.0f)
			{
				DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio stop sound(%d) %s\n", sound->getSoundId().getId(), sound->getTemplate()->getName()));

				if (sound->getTemplate()->getSoundCategory() == Audio::SC_voiceover)
				{
					Audio::unFadeAllNonVoiceover();
				}

				s_soundIdToSoundMap.erase(iterSoundIdToSoundMap);

				delete sound;
			}
		}
		else
		{
			DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio stop (KEEP ALIVE) sound(%d) %s\n", sound->getSoundId().getId(), sound->getTemplate()->getName()));
		}
	}
}

//-----------------------------------------------------------------------------
void AudioNamespace::removeSoundFromPrioritizedPlayingSounds(Sound2 const &sound)
{
	bool found = false;

	for(auto iterPrioritizedPlayingSounds = s_prioritizedPlayingSounds.begin(); iterPrioritizedPlayingSounds != s_prioritizedPlayingSounds.end(); ++iterPrioritizedPlayingSounds)
	{
		Sound2 const &currentSound = *(*iterPrioritizedPlayingSounds);

		if (currentSound.getSoundId().getId() == sound.getSoundId().getId())
		{
			found = true;
			s_prioritizedPlayingSounds.erase(iterPrioritizedPlayingSounds);
			DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Prioritized sound removed(%d) %s\n", currentSound.getSoundId().getId(), currentSound.getTemplate()->getName()));
			break;
		}
	}

	DEBUG_REPORT_LOG(!found, ("Prioritized sound not found(%s)\n", sound.getTemplate()->getName()));
}

// The sound category volume changes the volumes level of the specified sound category,
// this is a further 2d and 3d sound reduction on top of the setSoundVolume, [0...1]
//-----------------------------------------------------------------------------
void AudioNamespace::setSoundCategoryVolume(Audio::SoundCategory const soundCategory, float const volume)
{
#ifdef _DEBUG
	DEBUG_WARNING((volume < 0.0f), ("sound category volume(%f) < 0", volume));
	DEBUG_WARNING((volume > 1.0f), ("sound category volume(%f) > 1", volume));
#endif // _DEBUG

	s_soundCategoryVolumes[static_cast<unsigned int>(soundCategory)] = clamp(0.0f, volume, 1.0f);
}

// Returns the volume of the specified sound category, this is a further 2d and
// 3d sound reduction on top of the getSoundVolume(), [0...1]
//-----------------------------------------------------------------------------
float AudioNamespace::getSoundCategoryVolume(Audio::SoundCategory const soundCategory, bool settingOnly)
{
	float result = 1.0f;

	result = s_soundCategoryVolumes[static_cast<unsigned int>(soundCategory)];

	if (!settingOnly)
	{
		if (isNonBufferedMusic(soundCategory))
		{
			result *= s_nonBuffereMusicFadeVolume;
		}

		if (soundCategory != Audio::SC_backGroundMusic && soundCategory != Audio::SC_voiceover)
		{
			result *= s_globalAudioFadeVolume;
		}
		else if (soundCategory == Audio::SC_backGroundMusic && (s_nonBackgroundFadeCount == 0 || s_allAudioFadeCount != 0) )
		{
			result *= s_globalAudioFadeVolume;
		}
		else if (soundCategory == Audio::SC_voiceover && (s_nonBackgroundFadeCount != 0 || s_allAudioFadeCount != 0) )
		{
			result *= s_globalAudioFadeVolume;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool AudioNamespace::queueSample(SoundBucketList & soundBucketList, Sound2 & sound, float const distanceSquared, bool const soundIsAlreadyPlaying)
{
	bool result = true;

	// See if the sound already exists in the bucket
	auto iterSoundBucketList = soundBucketList.find(&sound.getTemplate()->getCrcName());

	if (iterSoundBucketList != soundBucketList.end())
	{
		SoundBucketData const & existingSound = iterSoundBucketList->second;
		bool playNewSound = false;

		if(distanceSquared < existingSound.m_distanceSquared || distanceSquared <= existingSound.m_distanceSquared && !sound.isInfiniteLooping())
		{
			playNewSound = true;
		}
		else if(!sound.isInfiniteLooping() && distanceSquared > 0.0f)
		{
			// The sound is further from the listener, but check to see if we want to play it anyways
			int const totalTimeMs = sound.getTotalTime();

			if (totalTimeMs > 0)
			{
				float const distancePercent = 1.0f - sqr(static_cast<float>(existingSound.m_distanceSquared) / static_cast<float>(distanceSquared));
				int const currentTimeMs = sound.getCurrentTime();
				float const playPercent = static_cast<float>(currentTimeMs) / static_cast<float>(totalTimeMs);

				playNewSound = (playPercent > distancePercent);
			}
		}

		// Sound already exists in the bucket, if the new sound is closer, play it and stop the previous sound
		if (playNewSound)
		{
			// Kill previous sound, since the new one is closer
			float const fadeOutTime = 0.0f;
			bool const keepAlive = iterSoundBucketList->second.m_sound->isInfiniteLooping();

			AudioNamespace::stopSound(iterSoundBucketList->second.m_sound->getSoundId(), fadeOutTime, keepAlive);
			//DEBUG_REPORT_LOG(true, ("Audio::queueSample() iterSoundBucketList->second.m_sound->endOfSample(%s)\n", iterSoundBucketList->second.m_sound->getTemplate()->getName()));

			// Insert the new sound
			iterSoundBucketList->second.m_sound = &sound;
			iterSoundBucketList->second.m_distanceSquared = distanceSquared;
			iterSoundBucketList->second.m_soundIsAlreadyPlaying = soundIsAlreadyPlaying;
		}
		else
		{
			// The new sound is NOT a better choice that what already exists
			result = false;
			//DEBUG_REPORT_LOG(true, ("Audio::queueSample() sound.endOfSample(%s)\n", sound.getTemplate()->getName()));
		}
	}
	else
	{
		// This sound is not in this bucket, add it
		soundBucketList.insert(std::make_pair(&sound.getTemplate()->getCrcName(), SoundBucketData(&sound, distanceSquared, soundIsAlreadyPlaying)));
		//DEBUG_REPORT_LOG(true, ("Audio::queueSample() soundBucketList.insert(%s)\n", sound.getTemplate()->getName()));
	}

	return result;
}

//-----------------------------------------------------------------------------

void AudioNamespace::insertionSort(QueuedSamplesToStartList & list, Sound2 & sound)
{
	int const newPriority = sound.getTemplate()->getPriority();
	float const newDistanceSquaredFromListener = sound.getDistanceSquaredFromListener();

	// Insert the new sound into the queue sorted by priority then distance

	auto iterQueuedSoundsToStartList = list.begin();

	for (; iterQueuedSoundsToStartList != list.end(); ++iterQueuedSoundsToStartList)
	{
		int const currentPriority = (*iterQueuedSoundsToStartList)->getTemplate()->getPriority();
		float const currentDistanceSquaredFromListener = (*iterQueuedSoundsToStartList)->getDistanceSquaredFromListener();

		if (newPriority < currentPriority || (newPriority == currentPriority && newDistanceSquaredFromListener < currentDistanceSquaredFromListener))
		{
			list.insert(iterQueuedSoundsToStartList, &sound);
			break;
		}
	}

	// If the item was not inserted previously, then it goes at the end of the list
	if (iterQueuedSoundsToStartList == list.end())
	{
		list.emplace_back(&sound);
	}
}

//-----------------------------------------------------------------------------
int AudioNamespace::getMaxNumberOfSamples()
{
	return std::min(Audio::getRequestedMaxNumberOfSamples(), Audio::getMaxDigitalMixerChannels());
}

//-----------------------------------------------------------------------------
FMOD_SPEAKERMODE  AudioNamespace::getProviderSpec(std::string const & provider)
{
	if (provider == "2 Speakers") return FMOD_SPEAKERMODE_STEREO;
	if (provider == "4 Speakers") return FMOD_SPEAKERMODE_QUAD;
	if (provider == "5.1 Speakers")	return FMOD_SPEAKERMODE_5POINT1;
	if (provider == "7.1 Speakers")	return FMOD_SPEAKERMODE_7POINT1;
	if (provider == "Dolby Surround") return FMOD_SPEAKERMODE_SURROUND;

	return FMOD_SPEAKERMODE_DEFAULT;
}

//-----------------------------------------------------------------------------
std::string const AudioNamespace::getSpeakerSpec()
{
	if (s_speakers == FMOD_SPEAKERMODE_DEFAULT)	return "Windows Speaker Configuration";
	if (s_speakers == FMOD_SPEAKERMODE_STEREO) return "2 Speakers";
	if (s_speakers == FMOD_SPEAKERMODE_QUAD) return "4 Speakers";
	if (s_speakers == FMOD_SPEAKERMODE_5POINT1)	return "5.1 Speakers";
	if (s_speakers == FMOD_SPEAKERMODE_7POINT1)	return "7.1 Speakers";
	if (s_speakers == FMOD_SPEAKERMODE_SURROUND) return "Dolby Surround";

	return "Unknown";
}

#ifdef _DEBUG
//-----------------------------------------------------------------------------

char const * const AudioNamespace::getSoundCategoryString(Audio::SoundCategory const soundCategory)
{
	switch (soundCategory)
	{
		case Audio::SC_ambient:         { return "<ambient>"; } break;
		case Audio::SC_explosion:       { return "<explosion>"; } break;
		case Audio::SC_item:            { return "<item>"; } break;
		case Audio::SC_movement:        { return "<movement>"; } break;
		case Audio::SC_userInterface:   { return "<userInterface>"; } break;
		case Audio::SC_vehicle:         { return "<vehicle>"; } break;
		case Audio::SC_vocalization:    { return "<vocalization>"; } break;
		case Audio::SC_weapon:          { return "<weapon>"; } break;
		case Audio::SC_backGroundMusic: { return "<backGroundMusic>"; } break;
		case Audio::SC_playerMusic:     { return "<playerMusic>"; } break;
		case Audio::SC_machine:         { return "<machine>"; } break;
		case Audio::SC_installation:    { return "<installation>"; } break;
		case Audio::SC_combatMusic:     { return "<combatMusic>"; } break;
		case Audio::SC_voiceover:       { return "<voiceover>"; } break;
		case Audio::SC_bufferedSound:   { return "<bufferedSound>"; } break;
		case Audio::SC_bufferedMusic:   { return "<bufferedMusic>"; } break;
		default: {}
	}

	return "";
}
#endif // _DEBUG

// ============================================================================
//
// Audio
//
// ============================================================================

#ifdef _DEBUG
//-----------------------------------------------------------------------------
void Audio::showAudioDebug()
{
	DEBUG_REPORT_PRINT(!s_audioEnabled, ("Audio Enabled              - no\n"));
	DEBUG_REPORT_PRINT(true, ("Miles Version              - %s\n", getFmodVersion().c_str()));
	DEBUG_REPORT_PRINT(true, ("Cache Size                 - %d KB\n", getCurrentCacheSize() / 1024));
	//DEBUG_REPORT_PRINT(true, ("Max Cached 2d Sample Size  - %d KB\n", getMaxCached2dSampleSize() / 1024));
	DEBUG_REPORT_PRINT(true, ("Cached Samples             - %d\n", getCachedSampleCount()));
	DEBUG_REPORT_PRINT(true, ("# of Sounds Created        - %d\n", getSoundCount()));
	DEBUG_REPORT_PRINT(true, ("# of Samples Playing       - %2d/max(%2d) requestedMax(%2d) milesMax(%2d)\n", static_cast<int>(s_prioritizedPlayingSounds.size()), getMaxNumberOfSamples(), Audio::getRequestedMaxNumberOfSamples(), Audio::getMaxDigitalMixerChannels()));
	DEBUG_REPORT_PRINT(true, ("Allocated Sound2d          - %d/%d peak\n", Sound2d::getCount(), Sound2d::getMaxCount()));
	DEBUG_REPORT_PRINT(true, ("Allocated Sound3d          - %d/%d peak\n", Sound3d::getCount(), Sound3d::getMaxCount()));
	DEBUG_REPORT_PRINT(true, ("CPU Usage/Latency          - %d%%/%d ms\n", s_digitalCpuPercent, s_digitalLatency));
	DEBUG_REPORT_PRINT(true, ("Sample Cache Hits/Misses   - %d/%d\n", getCacheHitCount(), getCacheMissCount()));
	DEBUG_REPORT_PRINT(true, ("Sound Instant Rejections   - %d\n", s_instantRejectionCount));
	DEBUG_REPORT_PRINT(true, ("Next sampleId/soundId      - %d/%d\n", s_nextSampleId, s_nextSoundId));
	DEBUG_REPORT_PRINT(true, ("Timer Delay (ms)           - %d/%d peak (%.1f) avg\n", s_timerCurrentDelay, s_timerHighestDelay, s_averageTimerDelay));
	DEBUG_REPORT_PRINT(true, ("User Speaker Setting       - %s\n", getCurrent3dProvider().c_str()));
	DEBUG_REPORT_PRINT(true, ("Miles Speaker Setting      - %s\n", getSpeakerSpec().c_str()));
	DEBUG_REPORT_PRINT(true, ("Environmental Reverb       - %s\n", getRoomTypeString()));
	DEBUG_REPORT_PRINT(true, ("Obstruction (interiors)    - %d%%\n", static_cast<int>(getObstruction() * 100.0f + 0.5f)));
	DEBUG_REPORT_PRINT(true, ("Occlusion (inside vs out)  - %d%%\n", static_cast<int>(getOcclusion() * 100.0f + 0.5f)));
	DEBUG_REPORT_PRINT(true, ("Listener Position          - (%.1f, %.1f, %.1f)\n", s_listener.m_positionCurrent.x, s_listener.m_positionCurrent.y, s_listener.m_positionCurrent.z));
	DEBUG_REPORT_PRINT(true, ("Falloff Power              - (%d)\n", s_soundFallOffPower));

	DEBUG_REPORT_PRINT(true, ("Non-voiceover fade count   - (%d)\n", s_nonVoiceoverFadeCount));
	DEBUG_REPORT_PRINT(true, ("Non-back music fade count  - (%d)\n", s_nonBackgroundFadeCount));
	DEBUG_REPORT_PRINT(true, ("All audio fade count       - (%d)\n", s_allAudioFadeCount));
	DEBUG_REPORT_PRINT(true, ("Non-buffered silenced      - (%s)\n", s_silenceNonBufferedMusic ? "true" : "false"));

	//DEBUG_REPORT_PRINT(true, ("Listener Translation       - (%.1f, %.1f, %.1f)\n", getListenerPosition().x, getListenerPosition().y, getListenerPosition().z));
	//DEBUG_REPORT_PRINT(true, ("Master Volume              - %.2f\n", getMasterVolume()));
	//DEBUG_REPORT_PRINT(true, ("Sound Effect Volume        - %.2f\n", getSoundEffectVolume()));
	//DEBUG_REPORT_PRINT(true, ("Background Music Volume    - %.2f\n", getBackGroundMusicVolume()));
	//DEBUG_REPORT_PRINT(true, ("Player Music Volume        - %.2f\n", getPlayerMusicVolume()));
	//DEBUG_REPORT_PRINT(true, ("User Interface Volume      - %.2f\n", getUserInterfaceVolume()));

	if (s_debugWindowPrioritizedSounds)
	{
		// Prioritized sounds

		DEBUG_REPORT_PRINT(true, ("** Prioritized Sounds Playing (%d) **\n", static_cast<int>(s_prioritizedPlayingSounds.size())));
		PrioritizedPlayingSounds::const_iterator iterPrioritizedPlayingSounds = s_prioritizedPlayingSounds.begin();

		for (; iterPrioritizedPlayingSounds != s_prioritizedPlayingSounds.end(); ++iterPrioritizedPlayingSounds)
		{
			Sound2 const *sound = (*iterPrioritizedPlayingSounds);

			bool const occluded = sound->isOccluded();
			bool obstructed = sound->isObstructed();

			DEBUG_REPORT_PRINT(true, ("%d %6.2f sid: %3d %s%s%s %s\n", sound->getTemplate()->getPriority() + 1, sound->getDistanceFromListener(), sound->getSoundId().getId(), occluded ? "OCC" : "", obstructed ? "OBS" : "", !occluded && !obstructed ? " NA" : "", sound->getTemplate()->getName()));
		}
	}

	if (s_debugWindowQueuedSounds)
	{
		// Show all the sounds queued in the audio system

		typedef std::map<std::string, int> SoundCount;
		static SoundCount soundCount;
		soundCount.clear();

		DEBUG_REPORT_PRINT(true, ("** Sounds Queued ** (%d)\n", static_cast<int>(s_soundIdToSoundMap.size())));
		SoundIdToSoundMap::iterator iterSoundIdToSoundMap = s_soundIdToSoundMap.begin();

		for (; iterSoundIdToSoundMap != s_soundIdToSoundMap.end(); ++iterSoundIdToSoundMap)
		{
			const char *path = iterSoundIdToSoundMap->second->getTemplate()->getName();

			SoundCount::iterator iterSoundCount = soundCount.find(path);

			if (iterSoundCount == soundCount.end())
			{
				soundCount.insert(std::make_pair(path, 1));
			}
			else
			{
				++(iterSoundCount->second);
			}
		}

		SoundCount::const_iterator iterSoundCount = soundCount.begin();

		for (; iterSoundCount != soundCount.end(); ++iterSoundCount)
		{
			DEBUG_REPORT_PRINT(true, ("%2dx %s\n", iterSoundCount->second, iterSoundCount->first.c_str()));
		}
	}

	if (s_debugWindowPlayingSamples)
	{
		// Show all the playing streamed samples

		DEBUG_REPORT_PRINT(true, ("** Stream Samples Playing ** (%d)\n", static_cast<int>(s_sampleIdToSampleStreamMap.size())));
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap = s_sampleIdToSampleStreamMap.begin();

		for (; iterSampleIdToSampleStreamMap != s_sampleIdToSampleStreamMap.end(); ++iterSampleIdToSampleStreamMap)
		{
			int const attenuation = static_cast<int>(iterSampleIdToSampleStreamMap->second.m_sound->getAttenuation() * 100.0f);
			int const volume = static_cast<int>(iterSampleIdToSampleStreamMap->second.m_sound->getTemplateVolume() * 100.0f);
			float const distance = iterSampleIdToSampleStreamMap->second.m_sound->getDistanceFromListener();
			std::string fileName = FileNameUtils::get(iterSampleIdToSampleStreamMap->second.getPath()->getString(), FileNameUtils::fileName | FileNameUtils::extension);

			DEBUG_REPORT_PRINT(true, ("v%%: %3d atn%%: %3d dis: %6.2f sid: %3d %s\n", volume, attenuation, distance, iterSampleIdToSampleStreamMap->second.m_sound->getSoundId().getId(), fileName.c_str()));
		}

		// Show all the playing 2d samples

		DEBUG_REPORT_PRINT(true, ("** 2d Samples Playing ** (%d)\n", static_cast<int>(s_sampleIdToSample2dMap.size())));
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap = s_sampleIdToSample2dMap.begin();

		for (; iterSampleIdToSample2dMap != s_sampleIdToSample2dMap.end(); ++iterSampleIdToSample2dMap)
		{
			int const attenuation = static_cast<int>(iterSampleIdToSample2dMap->second.m_sound->getAttenuation() * 100.0f);
			int const volume = static_cast<int>(iterSampleIdToSample2dMap->second.m_sound->getTemplateVolume() * 100.0f);
			float const distance = iterSampleIdToSample2dMap->second.m_sound->getDistanceFromListener();
			std::string fileName = FileNameUtils::get(iterSampleIdToSample2dMap->second.getPath()->getString(), FileNameUtils::fileName | FileNameUtils::extension);

			DEBUG_REPORT_PRINT(true, ("v%%: %3d atn%%: %3d dis: %6.2f sid: %3d %s\n", volume, attenuation, distance, iterSampleIdToSample2dMap->second.m_sound->getSoundId().getId(), fileName.c_str()));
		}

		// Show all the playing 3d samples

		DEBUG_REPORT_PRINT(true, ("** 3d Samples Playing ** (%d)\n", static_cast<int>(s_sampleIdToSample3dMap.size())));
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap = s_sampleIdToSample3dMap.begin();

		for (; iterSampleIdToSample3dMap != s_sampleIdToSample3dMap.end(); ++iterSampleIdToSample3dMap)
		{
			int const attenuation = static_cast<int>(iterSampleIdToSample3dMap->second.m_sound->getAttenuation() * 100.0f);
			int const volume = static_cast<int>(iterSampleIdToSample3dMap->second.m_sound->getTemplateVolume() * 100.0f);
			float const distance = iterSampleIdToSample3dMap->second.m_sound->getDistanceFromListener();
			std::string fileName = FileNameUtils::get(iterSampleIdToSample3dMap->second.getPath()->getString(), FileNameUtils::fileName | FileNameUtils::extension);

			DEBUG_REPORT_PRINT(true, ("v%%: %3d atn%%: %3d dis: %6.2f sid: %3d %s\n", volume, attenuation, distance, iterSampleIdToSample3dMap->second.m_sound->getSoundId().getId(), fileName.c_str()));
		}
	}
}

//-----------------------------------------------------------------------------

void Audio::debugDumpAudioText()
{
	s_debugDumpCachedSamplesTextFile = false;

	FILE *fp = fopen("audio.txt", "w+");

	if (fp != NULL)
	{
		fprintf(fp, "SWG Audio - Reference Counted Sound Cache (%s %s)\n", __DATE__, __TIME__);
		fprintf(fp, "%d samples @ %d KB\n", static_cast<int>(s_sampleCache.size()), Audio::getCurrentCacheSize() / 1024);
		fprintf(fp, "------------------\n");
		fprintf(fp, "\n");

		// Make an alphabetically sorted list of all the samples

		typedef std::map<std::string, SampleCacheEntry> TempMap;
		TempMap tempMap;

		SampleCache::const_iterator iterSampleCache = s_sampleCache.begin();

		for (; iterSampleCache != s_sampleCache.end(); ++iterSampleCache)
		{
			tempMap.insert(std::make_pair(iterSampleCache->first->getString(), iterSampleCache->second));
		}

		// Dump all the cached non-streamed samples

		int index = 1;
		iterSampleCache = s_sampleCache.begin();

		fprintf(fp, "** Cached Non-Streamed Samples **\n");
		fprintf(fp, "\n");

		TempMap::const_iterator iterTempMap = tempMap.begin();

		for (; iterTempMap != tempMap.end(); ++iterTempMap)
		{
			SampleCacheEntry const &sampleCacheEntry = iterTempMap->second;

			if (sampleCacheEntry.m_fileSize > 0)
			{
				fprintf(fp, "%4d ref cnt (%2d) (%4d KB) %s\n", index, sampleCacheEntry.m_referenceCount, sampleCacheEntry.m_fileSize / 1024, iterTempMap->first.c_str());
				++index;
			}
		}
		fprintf(fp, "\n");

		// Dump all the cached streamed samples

		index = 1;
		iterTempMap = tempMap.begin();

		fprintf(fp, "** Cached Streamed Samples **\n");
		fprintf(fp, "\n");

		for (; iterTempMap != tempMap.end(); ++iterTempMap)
		{
			SampleCacheEntry const &sampleCacheEntry = iterTempMap->second;

			if (sampleCacheEntry.m_fileSize <= 0)
			{
				fprintf(fp, "%4d (%2d) %s\n", index, sampleCacheEntry.m_referenceCount, iterTempMap->first.c_str());
				++index;
			}
		}

		fclose(fp);
		fp = NULL;
	}
}
#endif // _DEBUG

//-----------------------------------------------------------------------------

bool Audio::install()
{
	DEBUG_FATAL(s_installed, ("Already installed"));

	s_disableMiles = ConfigFile::getKeyBool("ClientAudio", "disableMiles", false);

	//====================================================================
	
	if (s_disableMiles)
	{
		REPORT_LOG(true, ("Audio: FMOD is disabled. To enable, set \"disableMiles=false\" in the [ClientAudio] section of client.cfg.\n"));
		return false;
	}
	
#ifdef _DEBUG
	DebugFlags::registerFlag(s_debugTimerDelay, "ClientAudio", "debugView_TimerDelay");
	DebugFlags::registerFlag(s_debugVisuals, "ClientAudio", "debugVisuals");
	DebugFlags::registerFlag(s_debugWindow, "ClientAudio", "debugWindow", Audio::showAudioDebug);
	DebugFlags::registerFlag(s_debugSoundStartStop, "ClientAudio", "debugView_SoundStartStop");
	DebugFlags::registerFlag(s_debugDumpCachedSamplesTextFile, "ClientAudio", "debugDump_audio.txt", Audio::debugDumpAudioText);
	DebugFlags::registerFlag(s_debugWindowPrioritizedSounds, "ClientAudio", "debugWindow_PrioritizedSounds");
	DebugFlags::registerFlag(s_debugWindowQueuedSounds, "ClientAudio", "debugWindow_QueuedSounds");
	DebugFlags::registerFlag(s_debugWindowPlayingSamples, "ClientAudio", "debugWindow_PlayingSamples");
	DebugFlags::registerFlag(s_consolidateQueuedSounds, "ClientAudio", "consolidateQueuedSounds");
#endif 

	REPORT_LOG(true, ("Audio: Starting intialization\n"));

	// Set the initial sound category volumes
	for (int i = 0; i < SC_count; ++i)
	{
		if (static_cast<SoundCategory>(i) == SC_backGroundMusic)
		{
			s_soundCategoryVolumes[i] = getDefaultBackGroundMusicVolume();
		}
		else if (static_cast<SoundCategory>(i) == SC_playerMusic)
		{
			s_soundCategoryVolumes[i] = getDefaultPlayerMusicVolume();
		}
		else if (static_cast<SoundCategory>(i) == SC_userInterface)
		{
			s_soundCategoryVolumes[i] = getDefaultUserInterfaceVolume();
		}
		else
		{
			s_soundCategoryVolumes[i] = 1.0f;
		}
	}

	s_allAudioFadeFactor = getDefaultFadeAllFactor();

	// Load the settings from the previous session
	const char * const section = "ClientAudio";
	LocalMachineOptionManager::registerOption(s_masterVolume,                             section, "masterVolume");
	LocalMachineOptionManager::registerOption(s_soundCategoryVolumes[SC_explosion],       section, "soundEffectVolume");
	LocalMachineOptionManager::registerOption(s_soundCategoryVolumes[SC_backGroundMusic], section, "backGroundMusicVolume");
	LocalMachineOptionManager::registerOption(s_soundCategoryVolumes[SC_playerMusic],     section, "playerMusicVolume");
	LocalMachineOptionManager::registerOption(s_soundCategoryVolumes[SC_userInterface],   section, "userInterfaceVolume");
	LocalMachineOptionManager::registerOption(s_soundCategoryVolumes[SC_ambient],         section, "ambientEffectVolume");
	LocalMachineOptionManager::registerOption(s_requestedMaxNumberOfSamples, section, "maxSampleCount");
	LocalMachineOptionManager::registerOption(s_audioEnabled,                             section, "enabled");
	LocalMachineOptionManager::registerOption(s_soundProvider, section, "soundProvider");
	LocalMachineOptionManager::registerOption(s_allAudioFadeFactor, section, "fadeFactor");

	REPORT_LOG(!s_audioEnabled, ("Audio: Audio is disabled. To enable audio, set \"enabled=true\" in the [ClientAudio] section of client.cfg, or enable audio in the game options menu.\n"));

	setRequestedMaxNumberOfSamples(s_requestedMaxNumberOfSamples);
	setMaxCached2dSampleSize(ConfigClientAudio::getMaxCached2dSampleSize());
	setSoundEffectVolume(s_soundCategoryVolumes[SC_explosion]);

	s_localPurgeList.clear();

	// Initialize FMOD
	REPORT_LOG(true, ("Audio: FMOD intialization\n"));

	s_maxDigitalMixerChannels = 64;

	// create studio + core system
	FMOD_RESULT fr = FMOD::Studio::System::create(&s_fmod_studio_system);

	if(fr != FMOD_OK)
	{
		REPORT_LOG(true, ("Audio: FMOD create error: (%d)\n", fr));
		remove();
		setEnabled(false);
		return false;
	}

	// get core system
	fr = s_fmod_studio_system->getCoreSystem(&s_fmod_core_system);

	if(fr != FMOD_OK)
	{
		REPORT_LOG(true, ("Audio: FMOD get core system error: (%d)\n", fr));
		remove();
		setEnabled(false);
		return false;
	}

	// set speaker config
	fr = s_fmod_core_system->setSoftwareFormat(getFrequency(), s_speakers, 0);

	if(fr != FMOD_OK)
	{
		REPORT_LOG(true, ("Audio: FMOD set speaker format error: (%d)\n", fr));
		remove();
		setEnabled(false);
		return false;
	}

	// set file callbacks
	fr = s_fmod_core_system->setFileSystem(fileOpenCallBack, fileCloseCallBack, fileReadCallBack, fileSeekCallBack, nullptr, nullptr, -1);

	if(fr != FMOD_OK)
	{
		REPORT_LOG(true, ("Audio: FMOD set file callbacks error: (%d)\n", fr));
		remove();
		setEnabled(false);
		return false;
	}
	
	// init
	fr = s_fmod_studio_system->initialize(s_maxDigitalMixerChannels, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);

	if(fr != FMOD_OK)
	{
		REPORT_LOG(true, ("Audio: FMOD initialize error: (%d)\n", fr));
		remove();
		setEnabled(false);
		return false;
	}
	
	s_listener.mFmodCoreSystem = s_fmod_core_system;
	s_listener.alter();

	//AIL_set_3D_rolloff_factor(s_digitalDevice2d, s_rollOffFactor); // TODO : set per channel ?

	REPORT_LOG(true, ("Audio: %s\n", getCurrent3dProvider().c_str()));
	REPORT_LOG(true, ("Audio: FMOD speakers are %s\n", getSpeakerSpec().c_str()));
	REPORT_LOG(true, ("Audio: FMOD Max Channels set to %d\n", getMaxDigitalMixerChannels()));

	s_obstruction = ConfigClientAudio::getObstruction();
	s_occlusion = ConfigClientAudio::getOcclusion();

	// Create the data table
	clearMusicOffsets();

	Iff dataTableIff;

	if (dataTableIff.open("datatables/music/music.iff", true))
	{
		delete s_musicDataTable;
		s_musicDataTable = new DataTable;
		s_musicDataTable->load(dataTableIff);

		// Store the row music name and its row index for fast indexing into the DataTable.
		const int rowCount = s_musicDataTable->getNumRows();

		for (int index = 0; index < rowCount; ++index)
		{
			std::string const &musicName = s_musicDataTable->getStringValue(0, index);

			s_musicOffsetMap.insert(std::make_pair(new PersistentCrcString(musicName.c_str(), true), index));
		}
	}

	AbstractFile::setAudioServe(serve);

	REPORT_LOG(true, ("Audio: Finished initializing\n"));
	DEBUG_REPORT_LOG (true, ("+=======================================================================\n"));

	s_audioServePerformanceTimer = new PerformanceTimer;

	s_installed = true;

	return true;
}

//-----------------------------------------------------------------------------

void Audio::remove()
{
	setRoomType(RT_generic);

#ifdef _DEBUG
	DebugFlags::unregisterFlag(s_debugTimerDelay);
	DebugFlags::unregisterFlag(s_debugVisuals);
	DebugFlags::unregisterFlag(s_debugWindow);
	DebugFlags::unregisterFlag(s_debugWindowPrioritizedSounds);
	DebugFlags::unregisterFlag(s_debugWindowQueuedSounds);
	DebugFlags::unregisterFlag(s_debugWindowPlayingSamples);
	DebugFlags::unregisterFlag(s_debugSoundStartStop);
	DebugFlags::unregisterFlag(s_debugDumpCachedSamplesTextFile);
#endif

	stopAllSounds();

#ifdef _DEBUG
	unsigned int const sample2dMapSize = static_cast<unsigned int>(s_sampleIdToSample2dMap.size());
	UNREF(sample2dMapSize);
	DEBUG_WARNING(!s_sampleIdToSample2dMap.empty(), ("Sample 2d map not empty"));
#endif
	s_sampleIdToSample2dMap.clear();

#ifdef _DEBUG
	unsigned int const sample3dMapSize = static_cast<unsigned int>(s_sampleIdToSample3dMap.size());
	UNREF(sample3dMapSize);
	DEBUG_WARNING(!s_sampleIdToSample3dMap.empty(), ("Sample 3d map not empty"));
#endif
	s_sampleIdToSample3dMap.clear();

#ifdef _DEBUG
	unsigned int const streamMapSize = static_cast<unsigned int>(s_sampleIdToSampleStreamMap.size());
	UNREF(streamMapSize);
	DEBUG_WARNING(!s_sampleIdToSampleStreamMap.empty(), ("Sample stream map not empty"));
#endif 
	s_sampleIdToSampleStreamMap.clear();

	s_localPurgeList.clear();

	// Shutdown FMOD
	if (s_installed)
	{
		s_installed = false;

		if(s_fmod_studio_system)
		{
			s_fmod_studio_system->unloadAll();
			s_fmod_studio_system->release();
			s_fmod_studio_system = nullptr;
		}
	}

#ifdef _DEBUG
	int const fileMapCount = static_cast<int>(s_fileMap.size());
	DEBUG_WARNING((fileMapCount > 0), ("File handles (%d) are still allocated.", fileMapCount));
#endif

	clearMusicOffsets();

	delete s_musicDataTable;
	s_musicDataTable = nullptr;

	// Delete all the reference counted samples
	for(auto& it : s_sampleCache)
	{
		// Delete the CrcString
		delete it.first;

		// Delete the sample data
		delete [] it.second.m_sampleRawData;
	}

	s_sampleCache.clear();

	delete s_audioServePerformanceTimer;
	s_audioServePerformanceTimer = nullptr;
}

//-----------------------------------------------------------------------------

bool Audio::isEnabled()
{
	return s_audioEnabled;
}

//-----------------------------------------------------------------------------

void Audio::setEnabled(bool const enabled)
{
	s_audioEnabled = enabled;
}

//-----------------------------------------------------------------------------

bool Audio::isMilesEnabled()
{
	return !s_disableMiles;
}

//-----------------------------------------------------------------------------

CrcString const *Audio::increaseReferenceCount(const char *path, bool const needToCacheSample)
{
	SampleCache::iterator iterSampleCache;
	CrcString const *result = AudioNamespace::cacheSound(TemporaryCrcString(path, true), iterSampleCache, needToCacheSample);

	return result;
}

//-----------------------------------------------------------------------------

void AudioNamespace::cacheSample(TemporaryCrcString const &path, AbstractFile *file, SampleCache::iterator &iterSampleCache)
{
	if (file != nullptr)
	{
		// Load and allocate the sample from disk
		const int fileSize = file->length();
		byte *fileImage = file->readEntireFileAndClose();

		if (fileImage != nullptr)
		{
			// Get the length of the sample
			float timeTotal = 0.0f;
			float timeCurrent = 0.0f;

			getSampleTime(path.getString(), fileImage, fileSize, timeTotal, timeCurrent);

			s_currentCacheSize += fileSize;

			iterSampleCache->second.m_sampleRawData = fileImage;
			iterSampleCache->second.m_fileSize = fileSize;
			iterSampleCache->second.m_time = timeTotal;

			DEBUG_REPORT_LOG(Audio::isDebugEnabled(), ("Audio: Sample cached (%5dk): size: %4dk %s (%s)\n", Audio::getCurrentCacheSize() / 1024, fileSize / 1024, path.getString(), Audio::getSampleType(fileImage, fileSize).c_str()));
		}
		else
		{
			DEBUG_WARNING(true, ("Error loading and allocating the sample: %s", path.getString()));
		}
	}
}

//-----------------------------------------------------------------------------

CrcString const *AudioNamespace::cacheSound(TemporaryCrcString const &path, SampleCache::iterator &iterSampleCache, bool const needToCacheSample)
{
	CrcString const *result = nullptr;

	if (s_installed && !path.isEmpty())
	{
		bool const cached = isCached(path, iterSampleCache);

		if (!cached || (needToCacheSample && iterSampleCache->second.m_sampleRawData == nullptr))
		{
			AbstractFile *file = TreeFile::open(path.getString(), AbstractFile::PriorityData, true);

			if (file != nullptr)
			{
				++s_cacheMissCount;

				if (cached)
				{
					// Since the sound was already cached but the sample was not, just cache the sample now
					cacheSample(path, file, iterSampleCache);
				}
				else
				{
					SampleCacheEntry sampleCacheEntry;

					// Save information about this sample
					sampleCacheEntry.setExtension(path.getString());

					std::pair<SampleCache::iterator, bool> resultPair = s_sampleCache.insert(std::make_pair(new PersistentCrcString(path.getString(), path.getCrc()), sampleCacheEntry));
					iterSampleCache = resultPair.first;

					DEBUG_FATAL((resultPair.second == false), ("Error putting the sample in the sound cache: %s", path.getString()));

					if (needToCacheSample)
					{
						cacheSample(path, file, iterSampleCache);
					}
				}

				++(iterSampleCache->second.m_referenceCount);
				//DEBUG_REPORT_LOG(Audio::isDebugEnabled(), ("Audio: Ref count increased: %d %s\n", iterSampleCache->second.m_referenceCount, path.getString()));

				result = iterSampleCache->first;

				delete file;
			}
			else
			{
				// The file must not exist on disk or it is not a sound file
				DEBUG_WARNING(true, ("The sound file does not exist: %s", path.getString()));
				result = nullptr;
			}
		}
		else
		{
			// Increase the reference count to this sample
			++(iterSampleCache->second.m_referenceCount);
			//DEBUG_REPORT_LOG(Audio::isDebugEnabled(), ("Audio: Ref count increased: %d %s\n", iterSampleCache->second.m_referenceCount, iterSampleCache->first->getString()));
			result = iterSampleCache->first;

			// Increase the number of cache hits
			++s_cacheHitCount;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool AudioNamespace::isNonBufferedMusic(Audio::SoundCategory const soundCategory)
{
	switch (soundCategory)
	{
		case Audio::SC_backGroundMusic:
		case Audio::SC_combatMusic:
		case Audio::SC_playerMusic:
		{
			return true;
		}
		default: return false;
	}
}

//-----------------------------------------------------------------------------

bool isCached(CrcString const &path, SampleCache::iterator &iterSampleCache)
{
	iterSampleCache = s_sampleCache.find(&path);

	return (iterSampleCache != s_sampleCache.end());
}

//-----------------------------------------------------------------------------

void Audio::decreaseReferenceCount(CrcString const &path)
{
	AudioNamespace::decreaseReferenceCount(path);
}

// Creates a SoundId to use to play a sound over and over.
//-----------------------------------------------------------------------------
SoundId Audio::createSoundId(char const *path)
{
	return SoundId(0, path);
}

// Allows a single instance of a sound to be played. If the sound is currently
// playing, it stops it and starts it again.
//-----------------------------------------------------------------------------
void Audio::playSound(SoundId &soundId, Vector const *position, CellProperty const * const parentCell)
{
	if (s_installed)
	{
		// If this sound is currently playing, stop it
		stopSound(soundId, 0.0f);

		// Start the sound back up and re-assign a the new sound id
		soundId = playSound(soundId.getPath().getString(), position, parentCell);
	}
}

//-----------------------------------------------------------------------------

void Audio::restartSound(SoundId &soundId, Vector const *position, float const fadeOutTime)
{
	if (s_installed)
	{
		soundId = restartSound(soundId.getPath().getString(), position, fadeOutTime);
	}
}

//-----------------------------------------------------------------------------

SoundId Audio::restartSound(char const *path, Vector const *position, float const fadeOutTime)
{
	SoundId result;

	if (s_installed)
	{
		// Build a list of all the sounds playing with the same path
		const TemporaryCrcString temporaryCrcString(path, true);

		for (SoundIdToSoundMap::const_iterator iterSoundIdToSoundMap = s_soundIdToSoundMap.begin(); iterSoundIdToSoundMap != s_soundIdToSoundMap.end(); ++iterSoundIdToSoundMap)
		{
			if (iterSoundIdToSoundMap->first.getPath() == temporaryCrcString)
			{
				s_localPurgeList.emplace_back(iterSoundIdToSoundMap->first);
			}
		}

		// Remove all the sounds playing with the same path
		for(auto& i : s_localPurgeList)
		{
			UNREF(fadeOutTime);
			stopSound(i, fadeOutTime);
		}

		s_localPurgeList.clear();

		// Start a new instance of the sound
		result = playSound(path, position, nullptr);
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId Audio::playSound(char const * const path)
{
	return playSound(path, nullptr, nullptr);
}

// This always plays the sound as a 2d sound.
//-----------------------------------------------------------------------------
SoundId Audio::playSound(char const *path, CellProperty const * const parentCell)
{
	return playSound(path, nullptr, parentCell);
}

// This could play the sound with either 2d or 3d depending on the file type
// specified. Sound Templates could either be 2d, 2d attenuated, or 3d.
//-----------------------------------------------------------------------------
//
SoundId Audio::playSound(char const *path, Vector const &position, CellProperty const * const parentCell)
{
	return playSound(path, &position, parentCell);
}

//-----------------------------------------------------------------------------

SoundId Audio::playSound(char const *path, Vector const * const position, CellProperty const * const parentCell)
{
	return AudioNamespace::playSound(path, position, nullptr, nullptr, parentCell);
}

//-----------------------------------------------------------------------------

SoundId playSound2d(SoundTemplate const *soundTemplate, Vector const * const position, CellProperty const * const parentCell)
{
	SoundId result;

	if (s_installed)
	{
		if (soundTemplate->is2d())
		{
			const auto* sound2dTemplate = dynamic_cast<Sound2dTemplate const *>(soundTemplate);
			NOT_NULL(sound2dTemplate);

			// Create the 2d sound and assign it an id
			result = SoundId(getNextSoundId(), (sound2dTemplate->getName() == nullptr) ? "" : sound2dTemplate->getName());

			DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio play sound 2d: %4d %s\n", result.getId(), soundTemplate->getName()));

			Sound2d *sound2d = new Sound2d(sound2dTemplate, result);

			if (sound2d == nullptr)
			{
				result.invalidate();

				DEBUG_REPORT_LOG(true, ("Audio: Sound is not playing because the memory block manager is full. %s\n", soundTemplate->getName()));
			}
			else
			{
				if (position != nullptr)
				{
					sound2d->setPosition_w(*position);
				}

				sound2d->setParentCell(parentCell);

				s_soundIdToSoundMap.insert(std::make_pair(result, sound2d));

				if (soundTemplate->getSoundCategory() == Audio::SC_voiceover)
				{
					Audio::fadeAllNonVoiceover();
				}

			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId playSound3d(SoundTemplate const *soundTemplate, Vector const &position, CellProperty const * const parentCell)
{
	SoundId result;

	if (s_installed)
	{
		if (soundTemplate->is3d())
		{
			const auto* sound3dTemplate = dynamic_cast<Sound3dTemplate const *>(soundTemplate);
			NOT_NULL(sound3dTemplate);

			// Create the 3d sound and assign it an id
			result = SoundId(getNextSoundId(), (soundTemplate->getName() == nullptr) ? "" : soundTemplate->getName());

			DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio play sound 3d: %4d %s\n", result.getId(), soundTemplate->getName()));

			Sound3d *sound3d = new Sound3d(sound3dTemplate, result);

			if (sound3d == nullptr)
			{
				result.invalidate();

				DEBUG_REPORT_LOG(true, ("Audio: Sound is not playing because the memory block manager is full. %s\n", soundTemplate->getName()));
			}
			else
			{
				sound3d->setPosition_w(position);
				sound3d->setParentCell(parentCell);

				s_soundIdToSoundMap.insert(std::make_pair(result, sound3d));

				if (soundTemplate->getSoundCategory() == Audio::SC_voiceover)
				{
					Audio::fadeAllNonVoiceover();
				}
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId attachSound(SoundTemplate const * soundTemplate, Object const * object, char const * hardPointName)
{
	SoundId result;

	if (s_installed && soundTemplate != nullptr && object != nullptr)
	{
		result = SoundId(getNextSoundId(), (soundTemplate->getName() == nullptr) ? "" : soundTemplate->getName());

		DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio attach sound(%d) %s\n", result.getId(), soundTemplate->getName()));

		if (soundTemplate->is3d())
		{
			const auto* sound3dTemplate = dynamic_cast<Sound3dTemplate const *>(soundTemplate);
			NOT_NULL(sound3dTemplate);

			// Create the 3d sound and assign it an id
			Sound3d *sound3d = new Sound3d(sound3dTemplate, result);

			if (sound3d == nullptr)
			{
				result.invalidate();

				DEBUG_REPORT_LOG(true, ("Audio: Sound is not playing because the memory block manager is full. %s\n", soundTemplate->getName()));
			}
			else
			{
				sound3d->setObject(object);
				sound3d->setHardPointName(hardPointName);

				s_soundIdToSoundMap.insert(std::make_pair(result, sound3d));

				if (soundTemplate->getSoundCategory() == Audio::SC_voiceover)
				{
					Audio::fadeAllNonVoiceover();
				}
			}
		}
		else
		{
			if (soundTemplate->is2d())
			{
				const auto* sound2dTemplate = dynamic_cast<Sound2dTemplate const *>(soundTemplate);
				NOT_NULL(sound2dTemplate);

				switch (sound2dTemplate->getAttenuationMethod())
				{
					case Audio::AM_2d:
					{
						// Create the 2d attenuated sound and assign it an id
						Sound2d *sound2d = new Sound2d(sound2dTemplate, result);

						if (sound2d == nullptr)
						{
							result.invalidate();

							DEBUG_REPORT_LOG(true, ("Audio: Sound is not playing because the memory block manager is full. %s\n", soundTemplate->getName()));
						}
						else
						{
							sound2d->setObject(object);
							sound2d->setHardPointName(hardPointName);

							s_soundIdToSoundMap.insert(std::make_pair(result, sound2d));

							if (soundTemplate->getSoundCategory() == Audio::SC_voiceover)
							{
								Audio::fadeAllNonVoiceover();
							}
						}
					}
					break;
					
					case Audio::AM_none:
					default:
					{
						DEBUG_WARNING(true, ("Trying to attach a non-3d or non-attenuated 2d sound to an object: %s", soundTemplate->getName()));
					}
					break;
				}
			}
		}
	}

	return result;
}

// Update all the sounds
//-----------------------------------------------------------------------------
void Audio::alter(float const deltaTime, Object const *listener)
{
	s_listenerObject = listener;

	if (!s_installed)
	{
		return;
	}

	{
		// Update the position of the listener
		{
			if (listener != nullptr)
			{
				s_listener.m_positionCurrent = listener->getPosition_w();
				s_listener.m_vectorForward = listener->getObjectFrameK_w();
				s_listener.m_vectorUp = listener->getObjectFrameJ_w();
			}

			s_listener.alter();
		}
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("queue existing");

		s_utilitySoundList.clear();
		{
			for(auto* sound : s_prioritizedPlayingSounds)
			{
				s_utilitySoundList.emplace_back(sound);
			}
		}

		// Insert all the current sounds into the consilidation buckets, if the sounds are in range
		{
			for(auto& iterUtilitySoundList : s_utilitySoundList)
			{
				Sound2 * sound = iterUtilitySoundList;
				bool const soundIsAlreadyPlaying = true;

				if ( sound->getDistanceSquaredFromListener() > sqr(sound->getDistanceAtVolumeCutOff()) || !queueSample(*sound, soundIsAlreadyPlaying))
				{
					// The sample is not a good choice to play so stop it
					Sound2 * sound2 = iterUtilitySoundList;
					float const fadeOutTime = 0.0f;
					bool const keepAlive = sound2->isInfiniteLooping();

					AudioNamespace::stopSound(sound2->getSoundId(), fadeOutTime, keepAlive);
				}
			}
		}
		
		s_utilitySoundList.clear();
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("update existing");

		// Update the sounds, samples get queued up in the alter calls if a sound is ready to play
		for(auto& iterSoundIdToSoundMap : s_soundIdToSoundMap)
		{
			Sound2 * const sound = iterSoundIdToSoundMap.second;

			if (sound != nullptr)
			{
				sound->alter(deltaTime);
			}
		}
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("build sample list");

		// Build the queued sample list from the consolidation buckets
		s_queuedSamplesToStartList.clear();

		// Center
		for(auto& iterCenterBucket : s_centerBucket)
		{
			insertionSort(s_queuedSamplesToStartList, *iterCenterBucket);
		}

		// Front
		for(auto& iterFrontBucket : s_frontBucket)
		{
			if (!iterFrontBucket.second.m_soundIsAlreadyPlaying)
			{
				insertionSort(s_queuedSamplesToStartList, *(iterFrontBucket.second.m_sound));
			}
		}

		// Back
		for(auto& iterBackBucket : s_backBucket)
		{
			if (!iterBackBucket.second.m_soundIsAlreadyPlaying)
			{
				insertionSort(s_queuedSamplesToStartList, *(iterBackBucket.second.m_sound));
			}
		}

		// Left
		for(auto& iterLeftBucket : s_leftBucket)
		{
			if (!iterLeftBucket.second.m_soundIsAlreadyPlaying)
			{
				insertionSort(s_queuedSamplesToStartList, *(iterLeftBucket.second.m_sound));
			}
		}

		// Right
		for(auto& iterRightBucket : s_rightBucket)
		{
			if (!iterRightBucket.second.m_soundIsAlreadyPlaying)
			{
				insertionSort(s_queuedSamplesToStartList, *(iterRightBucket.second.m_sound));
			}
		}

		// Clear the consolidation buckets
		s_centerBucket.clear();
		s_leftBucket.clear();
		s_rightBucket.clear();
		s_frontBucket.clear();
		s_backBucket.clear();
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("delete old sounds");

		// Delete any old sounds, if an object dies that had a sound attachment, the sound will die here
		for(auto& iterSoundIdToSoundMap : s_soundIdToSoundMap)
		{
			SoundId const &soundId = iterSoundIdToSoundMap.first;
			Sound2 * const sound = iterSoundIdToSoundMap.second;

			if ((sound != nullptr && sound->isDeletable()) || sound == nullptr)
			{
				s_localPurgeList.emplace_back(soundId);
			}
		}

		for(const auto& soundId : s_localPurgeList)
		{
			stopSound(soundId);
		}

		s_localPurgeList.clear();
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("replace sounds");

		// Check and see if the playing sounds need to be replaced by any new sounds that are queued to play
		if (static_cast<int>(s_prioritizedPlayingSounds.size() + s_queuedSamplesToStartList.size()) > getMaxNumberOfSamples())
		{
			// Populate the swap vector
			s_utilitySoundList.clear();

			//DEBUG_REPORT_LOG(s_debugWindow, ("Pre Sort\n"));
			for(auto& s_prioritizedPlayingSound : s_prioritizedPlayingSounds)
			{
				s_utilitySoundList.emplace_back(s_prioritizedPlayingSound);
				//DEBUG_REPORT_LOG(s_debugWindow, ("Priority: %d Distance: %.2f\n", (*iterPrioritizedPlayingSounds)->getTemplate()->getPriority(), (*iterPrioritizedPlayingSounds)->getDistanceSquaredFromListener()));
			}

			// See if the prioritized sounds are already sorted
			bool sorted = true;
			
			{
				for (unsigned int i = 1; i < s_utilitySoundList.size(); ++i)
				{
					int const currentPriority = s_utilitySoundList[i]->getTemplate()->getPriority();
					float const currentDistanceSquaredFromListener = s_utilitySoundList[i]->getDistanceSquaredFromListener();

					int const previousPriority = s_utilitySoundList[i - 1]->getTemplate()->getPriority();
					float const previousDistanceSquaredFromListener = s_utilitySoundList[i - 1]->getDistanceSquaredFromListener();

					if ((currentPriority < previousPriority) || ((currentPriority == previousPriority) && (currentDistanceSquaredFromListener < previousDistanceSquaredFromListener)))
					{
						sorted = false;
						break;
					}
				}
			}

			if (!sorted)
			{
				//DEBUG_REPORT_LOG(s_debugWindow, ("Sounds are not sorted, sorting.\n"));

				// Re-sort the prioritized playing sounds since they are not in the correct order due to the movement
				// of the listener
				for (unsigned int i = 0; i < s_utilitySoundList.size(); ++i)
				{
					for (unsigned int j = i + 1; j < s_utilitySoundList.size(); ++j)
					{
						int const currentPriority = s_utilitySoundList[j]->getTemplate()->getPriority();
						float const currentDistanceSquaredFromListener = s_utilitySoundList[j]->getDistanceSquaredFromListener();

						int const previousPriority = s_utilitySoundList[i]->getTemplate()->getPriority();
						float const previousDistanceSquaredFromListener = s_utilitySoundList[i]->getDistanceSquaredFromListener();

						if ((currentPriority < previousPriority) ||	((currentPriority == previousPriority) && (currentDistanceSquaredFromListener < previousDistanceSquaredFromListener)))
						{
							//DEBUG_REPORT_LOG(s_debugWindow, ("Swapping sound due to higher priority/distance: previous: %d %.2f current %d %.2f\n", previousPriority, previousDistanceSquaredFromListener, currentPriority, currentDistanceSquaredFromListener));

							// Swap current and previous
							Sound2 *temp = s_utilitySoundList[j];
							s_utilitySoundList[j] = s_utilitySoundList[i];
							s_utilitySoundList[i] = temp;
						}
					}
				}
			}
			else
			{
				//DEBUG_REPORT_LOG(s_debugWindow, ("Sounds are already sorted.\n"));
			}

			// Shove the re-sorted vector back into the list
			s_prioritizedPlayingSounds.clear();

			//DEBUG_REPORT_LOG(s_debugWindow, ("Post Sort\n"));
			for(auto& iterSwapVector : s_utilitySoundList)
			{
				NOT_NULL(iterSwapVector);
				s_prioritizedPlayingSounds.emplace_back(iterSwapVector);
				//DEBUG_REPORT_LOG(s_debugWindow, ("Priority: %d Distance: %.2f\n", (*iterSwapVector)->getTemplate()->getPriority(), (*iterSwapVector)->getDistanceSquaredFromListener()));
			}

			// Trim the lists
			for (;;)
			{
				if (static_cast<int>(s_prioritizedPlayingSounds.size() + s_queuedSamplesToStartList.size()) <= getMaxNumberOfSamples())
				{
					// Done trimming
					break;
				}

				// If the user switches the max number of sounds on the fly, we may need to trim the playing sounds
				if (static_cast<int>(s_prioritizedPlayingSounds.size()) > getMaxNumberOfSamples())
				{
					Sound2 * const sound = s_prioritizedPlayingSounds.back();
					bool const keepAlive = sound->isInfiniteLooping();
					AudioNamespace::stopSound(sound->getSoundId(), 0.0f, keepAlive);
				}
				else
				{
					DEBUG_FATAL(s_queuedSamplesToStartList.empty(), ("Empty queued sample list"));

					// Too many sounds to play, remove some more
					if (!s_prioritizedPlayingSounds.empty() && !s_queuedSamplesToStartList.empty())
					{
						// Remove a sound from the list which has the lowest priority/distance
						int const queuedPriority = s_queuedSamplesToStartList.back()->getTemplate()->getPriority();
						float const queuedDistanceSquaredFromListener = s_queuedSamplesToStartList.back()->getDistanceSquaredFromListener();

						if (s_prioritizedPlayingSounds.back()->getTemplate()->getPriority() == queuedPriority)
						{
							if (s_prioritizedPlayingSounds.back()->getDistanceSquaredFromListener() > queuedDistanceSquaredFromListener)
							{
								Sound2 * const sound = s_prioritizedPlayingSounds.back();
								bool const keepAlive = sound->isInfiniteLooping();
								AudioNamespace::stopSound(sound->getSoundId(), 0.0f, keepAlive);
							}
							else
							{
								Sound2 * const sound = s_queuedSamplesToStartList.back();
								float const fadeOutTime = 0.0f;
								bool const keepAlive = sound->isInfiniteLooping();

								AudioNamespace::stopSound(sound->getSoundId(), fadeOutTime, keepAlive);

								s_queuedSamplesToStartList.pop_back();
							}
						}
						else if (s_prioritizedPlayingSounds.back()->getTemplate()->getPriority() > queuedPriority)
						{
							// The playing sound has a worse priority then the queued sound, so stop the playing sound
							Sound2 * const sound = s_prioritizedPlayingSounds.back();
							bool const keepAlive = sound->isInfiniteLooping();
							AudioNamespace::stopSound(s_prioritizedPlayingSounds.back()->getSoundId(), 0.0f, keepAlive);
						}
						else
						{
							// The queued sound has a worse priority then the playing sound, so stop the queued sound
							Sound2 * const sound = s_queuedSamplesToStartList.back();
							float const fadeOutTime = 0.0f;
							bool const keepAlive = sound->isInfiniteLooping();

							AudioNamespace::stopSound(sound->getSoundId(), fadeOutTime, keepAlive);

							s_queuedSamplesToStartList.pop_back();
						}
					}
					else if (!s_queuedSamplesToStartList.empty())
					{
						Sound2 * const sound = s_queuedSamplesToStartList.back();
						float const fadeOutTime = 0.0f;
						bool const keepAlive = sound->isInfiniteLooping();

						AudioNamespace::stopSound(sound->getSoundId(), fadeOutTime, keepAlive);

						s_queuedSamplesToStartList.pop_back();
					}
					else if (!s_prioritizedPlayingSounds.empty())
					{
						DEBUG_FATAL(true, ("Somehow there is more prioritized sounds playing than the maximum number of sounds."));
					}
					else
					{
						DEBUG_FATAL(true, ("Somehow the maximum number of supported sounds is <= 0."));
					}
				}
			}
		}
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("start sounds");

		// Start the new queued sounds that did not get thrown out
		lock();

		for(auto* sound : s_queuedSamplesToStartList)
		{
			// Make sure the sound is still valid because it could have died between the time it was
			// inserted in the queue and the time it should start playing
			if (isSoundValid(sound->getSoundId()))
			{
				startSample(*sound);
			}
		}

		unLock();

		s_queuedSamplesToStartList.clear();

		DEBUG_FATAL(static_cast<int>(s_prioritizedPlayingSounds.size()) > getMaxNumberOfSamples(), ("Invalid number of sounds: %d max: %d", static_cast<int>(s_prioritizedPlayingSounds.size()), getMaxNumberOfSamples()));
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("debug visuals");
#ifdef _DEBUG
		// Add the debugging text

		SoundIdToSoundMap::iterator iterSoundIdToSoundMap = s_soundIdToSoundMap.begin();

		for (; iterSoundIdToSoundMap != s_soundIdToSoundMap.end(); ++iterSoundIdToSoundMap)
		{
			if (isDebugEnabled())
			{
				// Set the debug text

				char text[256];
				char const *soundTemplateName = iterSoundIdToSoundMap->second->getTemplate()->getName();
				char const *removeSoundSlash = strstr(soundTemplateName, "sound/");
				if (removeSoundSlash != NULL)
				{
					soundTemplateName = removeSoundSlash + strlen("sound/");
				}
				int const totalSampleSize = iterSoundIdToSoundMap->second->getTotalSampleSize();
				int const templateVolume = static_cast<int>(iterSoundIdToSoundMap->second->getTemplateVolume() * 100.0f);
				int const templatePitch = static_cast<int>(iterSoundIdToSoundMap->second->getPitchDelta() * 100.0f);
				float sampleTimeTotal = 0.0f;
				float sampleTimeCurrent = 0.0f;
				getCurrentSampleTime(iterSoundIdToSoundMap->first, sampleTimeTotal, sampleTimeCurrent);
				int const volumePercent = static_cast<int>(iterSoundIdToSoundMap->second->getAttenuation() * 100.0f);
				float const distanceFromListener = iterSoundIdToSoundMap->second->getDistanceFromListener();
				bool const occluded = iterSoundIdToSoundMap->second->isOccluded();
				bool const obstructed = iterSoundIdToSoundMap->second->isObstructed();
				float const distanceAtMaxVolume = iterSoundIdToSoundMap->second->getTemplate()->getDistanceAtMaxVolume();
				float const distanceAtVolumeCutOff = iterSoundIdToSoundMap->second->getDistanceAtVolumeCutOff();
				bool const outOfRange = (distanceFromListener >= distanceAtVolumeCutOff);
				TemporaryCrcString currentSample;
				getCurrentSample(iterSoundIdToSoundMap->first, currentSample);
				bool const streamed = iterSoundIdToSoundMap->second->isStreamed();
				AttenuationMethod attenuationMethod = iterSoundIdToSoundMap->second->getTemplate()->getAttenuationMethod();

				snprintf(text, sizeof(text), "%s %d %s\nv %d%% tv %d tp %d dis (%.1f, %.1f, %.1f)\n%s %s %s%s\n%s (%.2f/%.2f)", soundTemplateName, ((totalSampleSize < 1024) ? totalSampleSize : (totalSampleSize / 1024)), ((totalSampleSize < 1024) ? "bytes" : "KB"), volumePercent, templateVolume, templatePitch, distanceAtMaxVolume, distanceFromListener, distanceAtVolumeCutOff, getSoundCategoryString(iterSoundIdToSoundMap->second->getTemplate()->getSoundCategory()), AudioNamespace::getAttenuationMethodString(attenuationMethod), streamed ? "Streamed" : "Cached", outOfRange ? " OUT OF RANGE" : (occluded ? " OCC" : (obstructed ? " OBS" : "")), (strlen(currentSample.getString()) <= 0) ? "delayed" : currentSample.getString(), sampleTimeCurrent, sampleTimeTotal);
				iterSoundIdToSoundMap->second->setText(text, outOfRange ? VectorArgb::solidRed : ((occluded || obstructed) ? VectorArgb::solidCyan : VectorArgb::solidGreen));
			}
		}
#endif // _DEBUG
	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("global volume fade");

		float fadeTarget = 1.0f;

		if (s_nonVoiceoverFadeCount || s_nonBackgroundFadeCount)
		{
			static float const nonVoiceoverFadeTarget = 0.5f;
			static float const nonBackgroundFadeTarget = 0.0f;

			if ( s_nonBackgroundFadeCount )
				fadeTarget = fadeTarget < nonBackgroundFadeTarget ? fadeTarget : nonBackgroundFadeTarget;

			if ( s_nonVoiceoverFadeCount )
				fadeTarget = fadeTarget < nonVoiceoverFadeTarget ? fadeTarget : nonVoiceoverFadeTarget;
		}

		if(s_allAudioFadeCount)
			fadeTarget = fadeTarget < s_allAudioFadeFactor ? fadeTarget : s_allAudioFadeFactor;

		if (s_globalAudioFadeVolume < fadeTarget)
		{
			static float const fadeInRate = 1.5f;   // one over fade time

			s_globalAudioFadeVolume = clamp(0.0f, s_globalAudioFadeVolume + (deltaTime * fadeInRate), fadeTarget);
		}
		else if (s_globalAudioFadeVolume > fadeTarget)
		{
			static float const fadeOutRate = 10.0f;	// one over fade time

			s_globalAudioFadeVolume = clamp(fadeTarget, s_globalAudioFadeVolume - (deltaTime * fadeOutRate), 1.0f);
		}

	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("non-buffered music volume fade");

		float fadeTarget = 1.0f;

		if (s_silenceNonBufferedMusic)
			fadeTarget = 0.0f;

		if (s_nonBuffereMusicFadeVolume < fadeTarget)
		{
			float const fadeInRate = 1.5f;   // one over fade time

			s_nonBuffereMusicFadeVolume = clamp(0.0f, s_nonBuffereMusicFadeVolume + (deltaTime * fadeInRate), fadeTarget);
		}
		else if (s_nonBuffereMusicFadeVolume > fadeTarget)
		{
			float const fadeOutRate = 10.0f;	// one over fade time

			s_nonBuffereMusicFadeVolume = clamp(fadeTarget, s_nonBuffereMusicFadeVolume - (deltaTime * fadeOutRate), 1.0f);
		}

	}

	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("fmod update()");

		s_fmod_studio_system->update();
	}
}

//-----------------------------------------------------------------------------

void Audio::setSamplePosition_w(SampleId const &sampleId, Vector const &position_w)
{
	const SampleIdToSample3dMap::const_iterator it = s_sampleIdToSample3dMap.find(sampleId);

	if (it != s_sampleIdToSample3dMap.end())
	{
		FMOD_VECTOR pos = { position_w.x, position_w.y, position_w.z };
		FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

		if(it->second.mFmodChannel)
		{
			it->second.mFmodChannel->set3DAttributes(&pos, &vel);
		}
		else
		{
			REPORT_LOG(true, ("Audio: setsamplepos no channel\n"));
		}
	}
}

//-----------------------------------------------------------------------------

std::string Audio::getFmodVersion()
{
	char version[256];
	unsigned int nr = 0;
	
	s_fmod_core_system->getVersion(&nr);
	sprintf(version, "%x", nr);

	return version;
}

//-----------------------------------------------------------------------------

int getFrequency()
{
	return 22050;
}

//-----------------------------------------------------------------------------

int getChannels()
{
	return 2;
}

//-----------------------------------------------------------------------------

int Audio::getCurrentCacheSize()
{
	return s_currentCacheSize;
}

//-----------------------------------------------------------------------------

int Audio::getCacheHitCount()
{
	return s_cacheHitCount;
}

//-----------------------------------------------------------------------------

int Audio::getCacheMissCount()
{
	return s_cacheMissCount;
}

//-----------------------------------------------------------------------------

std::vector<std::string> Audio::get3dProviders()
{
	std::vector<std::string> providers;

	for(const auto& it : s_3dProviderMap)
	{
		if(it.second.m_supported)
		{
			providers.emplace_back(it.second.m_name);
		}
	}

	return providers;
}

//-----------------------------------------------------------------------------

std::string Audio::getSampleType(void *fileImage, int fileSize)
{
	std::string text;

	if (s_installed)
	{
		// Get the file size
		if (fileImage && fileSize > 0)
		{
			text = "Standard PCM wav file";
			// Get the file type
		/*	S32 fileType = AIL_file_type(fileImage, fileSize);

			// Set the file type string
			switch (fileType)
			{
				case AILFILETYPE_PCM_WAV:
					{
						text = "Standard PCM wav file";
					}
					break;
				case AILFILETYPE_ADPCM_WAV:
					{
						text = "IMA ADPCM compressed wave file";
					}
					break;
				case AILFILETYPE_OTHER_WAV:
					{
						text = "Unsupported compressed wave";
					}
					break;
				case AILFILETYPE_OTHER_ASI_WAV:
					{
						text = "Compressed wave file that is handled by an unspecified ASI decoder";
					}
					break;
				case AILFILETYPE_VOC:
					{
						text = "Creative VOC digital sound file";
					}
					break;
				case AILFILETYPE_DLS:
					{
						text = "Uncompressed DLS file";
					}
					break;
				case AILFILETYPE_MLS:
					{
						text = "Compressed DLS file";
					}
					break;
				case AILFILETYPE_MPEG_L1_AUDIO:
					{
						text = "Compressed MPEG Layer 1 file (which MSS cannot play)";
					}
					break;
				case AILFILETYPE_MPEG_L2_AUDIO:
					{
						text = "Compressed MPEG Layer 2 file (which MSS cannot play)";
					}
					break;
				case AILFILETYPE_MPEG_L3_AUDIO:
					{
						text = "Compressed MPEG Layer 3 file";
					}
					break;
				default:
					{
						DEBUG_FATAL(true, ("Audio::getSampleType() - Unsupported file type."));
					}
				case AILFILETYPE_UNKNOWN:
					{
						text = "The file is not one of the supported types";
					}
					break;
			}*/
		}
		else
		{
			text = "File does not exist on disk";
		}
	}

	return text;
}

//-----------------------------------------------------------------------------

int Audio::getSampleSize(char const *path)
{
	int const result = TreeFile::getFileSize(path);

	// If you hit this error, you should probably fix the code that is calling this
	DEBUG_WARNING(!DataLint::isEnabled() && (result == -1), ("Could not get sample size for %s", path));

	return result;
}

// Stops a sound in the requested time. The sound will then be removed from the
// sound system which invalidates the sound id.
//-----------------------------------------------------------------------------
void Audio::stopSound(SoundId const &soundId, float const fadeOutTime)
{
	AudioNamespace::stopSound(soundId, fadeOutTime, false);
}

// Get the status of the sound. If a sound id is no longer valid, PS_done is returned.
//-----------------------------------------------------------------------------
Audio::PlayBackStatus Audio::getSoundPlayBackStatus(SoundId const &soundId)
{
	PlayBackStatus result = PS_doesNotExist;

	if (s_installed)
	{
		auto soundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

		if (soundIdToSoundMap != s_soundIdToSoundMap.end())
		{
			NOT_NULL(soundIdToSoundMap->second);

			if (soundIdToSoundMap->second->isPlaying())
			{
				result = PS_playing;
			}
			else
			{
				result = PS_done;
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

Audio::PlayBackStatus Audio::getSamplePlayBackStatus(SampleId const &sampleId)
{
	PlayBackStatus result = PS_done;

	if (s_installed)
	{
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		int status = PS_done;
		PlayBackStatus samplePlayBackStatus = PS_done;

		if (isSample2d(sampleId, iterSampleIdToSample2dMap))
		{
			status = iterSampleIdToSample2dMap->second.getChannelStatus();
			samplePlayBackStatus = iterSampleIdToSample2dMap->second.m_status;
		}
		else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
		{
			status = iterSampleIdToSample3dMap->second.getChannelStatus();
			samplePlayBackStatus = iterSampleIdToSample3dMap->second.m_status;
		}
		else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
		{
			status = iterSampleIdToSampleStreamMap->second.getChannelStatus();
			samplePlayBackStatus = iterSampleIdToSampleStreamMap->second.m_status;
		}

		switch (status)
		{
			case Sample2d::ChannelReady: { result = samplePlayBackStatus;} break;
			case Sample2d::ChannelPlaying:	{ result = PS_playing; } break;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool Audio::isSampleValid(SampleId const &sampleId)
{
	bool result = false;

	if (s_installed)
	{
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		if (isSample2d(sampleId, iterSampleIdToSample2dMap))
		{
			result = true;
		}
		else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
		{
			result = true;
		}
		else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
		{
			result = true;
		}
	}

	return result;
}

//----------------------------------------------------------------------

bool Audio::isSampleForSoundIdPlaying (const SoundId & soundId)
{
	const Sound2 * const sound = getSoundById(soundId);
	
	if (sound)
		return isSampleValid(sound->getSampleId());

	return false;
}

//-----------------------------------------------------------------------------

bool Audio::isSoundValid(SoundId const &soundId)
{
	bool result = false;

	if (s_installed)
	{
		const auto it = s_soundIdToSoundMap.find(soundId);

		if (it != s_soundIdToSoundMap.end())
		{
			result = true;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::startSample(Sound2 &sound)
{
	NP_PROFILER_AUTO_BLOCK_DEFINE("Audio::startSample");
	bool result = true;

	SampleId const sampleId(createSampleId(sound));

	if (sampleId.getId() == 0)
	{
		DEBUG_WARNING(true, ("Unable to create a valid sample id. All the samples must be occupied."));

		float const fadeOutTime = 0.0f;
		bool const keepAlive = sound.isInfiniteLooping();

		AudioNamespace::stopSound(sound.getSoundId(), fadeOutTime, keepAlive);
		return;
	}

	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("Audio::startSample::isSample2d");
		NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);

		// Get the sample from the cache
		auto iterSample = s_sampleCache.find(iterSampleIdToSample2dMap->second.getPath());

		if (iterSample != s_sampleCache.end())
		{
			DEBUG_FATAL((iterSample->second.m_fileSize == 0), ("File size is 0 %s", iterSampleIdToSample2dMap->second.getPath()));

			FMOD_RESULT fr = s_fmod_core_system->playSound(iterSampleIdToSample2dMap->second.mFmodSample, nullptr, true, &iterSampleIdToSample2dMap->second.mFmodChannel);

			if(fr == FMOD_OK)
			{
				setSampleVolume(iterSampleIdToSample2dMap->first, sound.getVolume());

				sound.setPlayBackRate(getSamplePlayBackRate(iterSampleIdToSample2dMap->first));

				setSamplePlayBackRate(iterSampleIdToSample2dMap->first, sound.getPlayBackRate(), sound.getPlayBackRateDelta());

				// Set the loop points
				int loopStartOffset = 0;
				int loopEndOffset = 0;

				if (getLoopOffsets(*iterSampleIdToSample2dMap->second.getPath(), loopStartOffset, loopEndOffset))
				{
					iterSampleIdToSample2dMap->second.mFmodChannel->setLoopPoints(static_cast<unsigned int>(loopStartOffset), FMOD_TIMEUNIT_MS, static_cast<unsigned int>(loopEndOffset), FMOD_TIMEUNIT_MS);
				}

				// Does this sample loop forever? Telling Miles lets us not have a hitch at the loop point.
				int const loopCount = iterSampleIdToSample2dMap->second.m_sound->getLoopCount();
				bool const infiniteLooping = iterSampleIdToSample2dMap->second.m_sound->getTemplate()->isInfiniteLooping();
				bool const noDelay = (iterSampleIdToSample2dMap->second.m_sound->getTemplate()->getLoopDelayMax() <= 0.0f);
				int const sampleCount = iterSampleIdToSample2dMap->second.m_sound->getTemplate()->getSampleCount();

				if (infiniteLooping && noDelay && (sampleCount <= 1))
				{
					// Specify infinite looping
					iterSampleIdToSample2dMap->second.mFmodChannel->setLoopCount(-1);
				}
				else
				{
					if (loopCount >= 1 && noDelay && sampleCount <= 1)
					{
						// Specify the loop count
						iterSampleIdToSample2dMap->second.mFmodChannel->setLoopCount(loopCount);
					}

					// Register the end of sample callback
					iterSampleIdToSample2dMap->second.mFmodChannel->setCallback(&endOfSample2dCallBack);
				}

				// Play the sample
				fr = iterSampleIdToSample2dMap->second.mFmodChannel->setPaused(false);

				if(fr != FMOD_OK)
				{
					REPORT_LOG(true, ("Audio: failed playback\n"));
				}
				
				sound.setSampleId(sampleId);

				addPlayingSound(sound);
			}
			else
			{
				DEBUG_WARNING(true, ("Error preparing 2D sample for playback: %s\n", iterSampleIdToSample2dMap->second.getPath()->getString()));

				result = false;
			}
		}
		else
		{
			DEBUG_WARNING(true, ("Unable to find the sample in the sound cache: %s\n", iterSampleIdToSample2dMap->second.getPath()->getString()));
			result = false;
		}
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("Audio::startSample::isSample3d");
		NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);

		// Get the sample from the cache
		auto iterSample = s_sampleCache.find(iterSampleIdToSample3dMap->second.getPath());

		if (iterSample != s_sampleCache.end())
		{
			FMOD_RESULT fr = s_fmod_core_system->playSound(iterSampleIdToSample3dMap->second.mFmodSample, nullptr, true, &iterSampleIdToSample3dMap->second.mFmodChannel);

			if(fr == FMOD_OK)
			{
				setSampleVolume(iterSampleIdToSample3dMap->first, sound.getVolume());
				setSampleOcclusion(iterSampleIdToSample3dMap->first, sound.getOcclusion());
				setSampleObstruction(iterSampleIdToSample3dMap->first, sound.getObstruction());

				sound.setPlayBackRate(getSamplePlayBackRate(iterSampleIdToSample3dMap->first));
				setSamplePlayBackRate(iterSampleIdToSample3dMap->first, sound.getPlayBackRate(), sound.getPlayBackRateDelta());

				// Does this sample loop forever? Telling Miles lets us not have a hitch at the loop point.
				int const loopCount = iterSampleIdToSample3dMap->second.m_sound->getLoopCount();
				bool const infiniteLooping = iterSampleIdToSample3dMap->second.m_sound->isInfiniteLooping();
				bool const noDelay = (iterSampleIdToSample3dMap->second.m_sound->getTemplate()->getLoopDelayMax() <= 0.0f);
				int const sampleCount = iterSampleIdToSample3dMap->second.m_sound->getTemplate()->getSampleCount();

				if (infiniteLooping && noDelay && (sampleCount <= 1))
				{
					// Specify infinite looping
					iterSampleIdToSample3dMap->second.mFmodChannel->setLoopCount(-1);
				}
				else
				{
					if (loopCount >= 1 && noDelay && sampleCount <= 1)
					{
						// Specify the loop count
						iterSampleIdToSample3dMap->second.mFmodChannel->setLoopCount(loopCount);
					}

					// Register the end of sample callback
					iterSampleIdToSample3dMap->second.mFmodChannel->setCallback(&endOfSample3dCallBack);
				}

				Vector const position(iterSampleIdToSample3dMap->second.m_sound->getPosition_w());

				FMOD_VECTOR pos = { position.x, position.y, position.z };
				FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
				
				iterSampleIdToSample3dMap->second.mFmodChannel->set3DAttributes(&pos, &vel);

				// Set the audible distances
				float distanceAtMaxVolume = iterSampleIdToSample3dMap->second.m_sound->getTemplate()->getDistanceAtMaxVolume();

				bool fixDistanceMin = false;

				if (distanceAtMaxVolume <= 0.0f)
				{
					fixDistanceMin = true;
				}

				if (fixDistanceMin)
				{
					DEBUG_WARNING(true, ("sound distance at max volume (%f) should be greater greater than zero for \"%s\".", distanceAtMaxVolume, iterSampleIdToSample3dMap->second.getPath()));
					distanceAtMaxVolume = 2.0f;
				}

				float const distanceMin = distanceAtMaxVolume;
				float const distanceMax = getFallOffDistance(distanceMin);

				iterSampleIdToSample3dMap->second.mFmodChannel->set3DMinMaxDistance(distanceMin, distanceMax);

				// Play the sample
				fr = iterSampleIdToSample3dMap->second.mFmodChannel->setPaused(false);

				if(fr != FMOD_OK)
				{
					REPORT_LOG(true, ("Audio: failed playback\n"));
				}

				sound.setSampleId(sampleId);

				addPlayingSound(sound);
			}
			else
			{
				DEBUG_REPORT_LOG(true, ("  Sample:    %s\n", iterSample->first->getString()));
				DEBUG_REPORT_LOG(true, ("  FileSize:  %d\n", iterSample->second.m_fileSize));
				DEBUG_REPORT_LOG(true, ("  Time:      %f\n", iterSample->second.m_time));
				DEBUG_REPORT_LOG(true, ("  Ref Count: %d\n", iterSample->second.m_referenceCount));

				result = false;
			}
		}
		else
		{
			DEBUG_WARNING(true, ("Unable to find the sample in the sound cache: \"%s\"", iterSampleIdToSample3dMap->second.getPath()->getString()));
			result = false;
		}
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		NP_PROFILER_AUTO_BLOCK_DEFINE("Audio::startSample::isSampleStream");
		// The number of streamed samples is further limited due to disk access
		if (iterSampleIdToSampleStreamMap->second.mFmodStream != nullptr)
		{
			FMOD_RESULT fr = s_fmod_core_system->playSound(iterSampleIdToSampleStreamMap->second.mFmodStream, nullptr, true, &iterSampleIdToSampleStreamMap->second.mFmodChannel);

			if(fr == FMOD_OK)
			{
				setSampleVolume(iterSampleIdToSampleStreamMap->first, sound.getVolume());

				sound.setPlayBackRate(getSamplePlayBackRate(iterSampleIdToSampleStreamMap->first));
				setSamplePlayBackRate(iterSampleIdToSampleStreamMap->first, sound.getPlayBackRate(), sound.getPlayBackRateDelta());
				
				// Set the loop points
				int loopStartOffset = 0;
				int loopEndOffset = 0;

				if(getLoopOffsets(*iterSampleIdToSampleStreamMap->second.getPath(), loopStartOffset, loopEndOffset))
				{
					iterSampleIdToSampleStreamMap->second.mFmodChannel->setLoopPoints(static_cast<unsigned int>(loopStartOffset), FMOD_TIMEUNIT_MS, static_cast<unsigned int>(loopEndOffset), FMOD_TIMEUNIT_MS);
				}

				DEBUG_REPORT_LOG(s_debugSoundStartStop, ("Audio start stream: %s\n", iterSampleIdToSampleStreamMap->second.getPath()->getString()));

				// Does this sample loop forever? Telling Miles lets us not have a hitch at the loop point.
				int const loopCount = iterSampleIdToSampleStreamMap->second.m_sound->getLoopCount();
				bool const infiniteLooping = (loopCount <= -1);
				bool const noDelay = (iterSampleIdToSampleStreamMap->second.m_sound->getTemplate()->getLoopDelayMax() <= 0.0f);
				int const sampleCount = iterSampleIdToSampleStreamMap->second.m_sound->getTemplate()->getSampleCount();

				// This is being changed to see if I can handle the looping using callbacks and it still not have a hitch
				if (infiniteLooping && noDelay && (sampleCount <= 1))
				{
					// Specify infinite looping
					iterSampleIdToSampleStreamMap->second.mFmodChannel->setLoopCount(-1);
				}
				else
				{
					if ((loopCount >= 1) && noDelay && (sampleCount <= 1))
					{
						// Specify the loop count
						iterSampleIdToSampleStreamMap->second.mFmodChannel->setLoopCount(loopCount);
					}

					// Register the end of sample callback
					iterSampleIdToSampleStreamMap->second.mFmodChannel->setCallback(&endOfSampleStreamCallBack);
				}

				fr = iterSampleIdToSampleStreamMap->second.mFmodChannel->setPaused(false);

				if(fr != FMOD_OK)
				{
					REPORT_LOG(true, ("Audio: failed playback\n"));
				}

				sound.setSampleId(sampleId);

				addPlayingSound(sound);
			}
		}
		else
		{
			DEBUG_WARNING(true, ("Sound stream is NULL\n"));

			// This should not happen but lets be graceful and not crash
			s_sampleIdToSampleStreamMap.erase(iterSampleIdToSampleStreamMap);
			result = false;
		}
	}
	else
	{
		result = false;
		DEBUG_WARNING(true, ("Invalid sampleId specified: %d", sound.getSampleId()));
	}

	// If there was an error, release this sampleId
	if (!result)
	{
		DEBUG_REPORT_LOG(true, ("Unable to start the sample...releasing: %s\n", sound.getTemplate()->getName()));

		float const fadeOutTime = 0.0f;
		bool const keepAlive = sound.isInfiniteLooping();

		AudioNamespace::stopSound(sound.getSoundId(), fadeOutTime, keepAlive);
	}
}

//-----------------------------------------------------------------------------

void Audio::setSampleOcclusion(SampleId const &sampleId, float const occlusion)
{
	DEBUG_WARNING((occlusion < 0.0f) || (occlusion > 1.0f), ("occlusion(%f) must be [0...1]", occlusion));

	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;

	if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		Sample3d const &sample3d = iterSampleIdToSample3dMap->second;

		if(sample3d.mFmodChannel)
		{
			sample3d.mFmodChannel->set3DOcclusion(clamp(0.0f, occlusion, 1.0f), clamp(0.0f, occlusion, 1.0f));
		}
		else
		{
			REPORT_LOG(true, ("Audio: failed setting occlusion: no channel.\n"));
		}
	}
}

//-----------------------------------------------------------------------------

void Audio::setSampleObstruction(SampleId const &sampleId, float const obstruction)
{
	DEBUG_WARNING((obstruction < 0.0f) || (obstruction > 1.0f), ("obstruction(%f) must be [0...1]", obstruction));

	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;

	if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		Sample3d const &sample3d = iterSampleIdToSample3dMap->second;

		
		if(sample3d.mFmodChannel)
		{
			// nope
		}
		else
		{
			REPORT_LOG(true, ("Audio: failed setting occlusion: no channel.\n"));
		}
	}
}

//-----------------------------------------------------------------------------

void Audio::setSampleVolume(SampleId const &sampleId, float const volume)
{
#ifdef _DEBUG
	//static int count = 0;
	DEBUG_WARNING((volume < 0.0f), ("volume(%f) must be [0...1]", volume));
	DEBUG_WARNING((volume > 1.0f), ("volume(%f) must be [0...1]", volume));
#endif // _DEBUG

	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);
		NOT_NULL(iterSampleIdToSample2dMap->second.mFmodChannel);

		float const finalVolume = !s_audioEnabled ? 0.0f : clamp(0.0f, volume * s_masterVolume * getSoundCategoryVolume(iterSampleIdToSample2dMap->second.m_sound->getTemplate()->getSoundCategory()), 1.0f);

		Sample2d const &sample2d = iterSampleIdToSample2dMap->second;

		sample2d.mFmodChannel->setVolume(finalVolume);
		sample2d.mFmodChannel->setReverbProperties(0, 0.0f);
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);
		NOT_NULL(iterSampleIdToSample3dMap->second.mFmodChannel);

		static float finalVolume;

		finalVolume = !s_audioEnabled ? 0.0f : clamp(0.0f, volume * s_masterVolume * getSoundCategoryVolume(iterSampleIdToSample3dMap->second.m_sound->getTemplate()->getSoundCategory()), 1.0f);

		// This is a hack because the EAX providers sometimes play sounds at full volume
		// even when passing in a small number for the volume
		if (finalVolume < 0.01)
		{
			finalVolume = 0.0f;
		}

		Sample3d const &sample3d = iterSampleIdToSample3dMap->second;

		sample3d.mFmodChannel->setVolume(finalVolume);
		sample3d.mFmodChannel->setReverbProperties(0, 0.0f);
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodStream);
		NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodChannel);

		float const finalVolume = !s_audioEnabled ? 0.0f : clamp(0.0f, volume * s_masterVolume * getSoundCategoryVolume(iterSampleIdToSampleStreamMap->second.m_sound->getTemplate()->getSoundCategory()), 1.0f);

		SampleStream const &sampleStream = iterSampleIdToSampleStreamMap->second;

		sampleStream.mFmodChannel->setVolume(finalVolume);
		sampleStream.mFmodChannel->setReverbProperties(0, 0.0f);
	}
}

//-----------------------------------------------------------------------------

void Audio::setSamplePlayBackRate(SampleId const &sampleId, int const playBackRate, float const playBackRateDelta)
{
	DEBUG_FATAL((playBackRate <= 0), ("Invalid initial playback rate: %d", playBackRate));

	if (s_installed && playBackRateDelta > 0.0f)
	{
		int const finalPlayBackRate = static_cast<int>(static_cast<float>(playBackRate) * playBackRateDelta);
		static int const minPlayBackRate = 5000;
		static int const maxPlayBackRate = 60000;

		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		int const clampedPlayBackRate = clamp(minPlayBackRate, finalPlayBackRate, maxPlayBackRate);

#ifdef _DEBUG
		CrcString const * fileName = 0;
#endif

		if (isSample2d(sampleId, iterSampleIdToSample2dMap))
		{
#ifdef _DEBUG
			fileName = iterSampleIdToSample2dMap->second.getPath();
#endif
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodChannel);

			iterSampleIdToSample2dMap->second.mFmodChannel->setFrequency(static_cast<float>(clampedPlayBackRate));
		}
		else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
		{
#ifdef _DEBUG
			fileName = iterSampleIdToSample3dMap->second.getPath();
#endif
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodChannel);

			iterSampleIdToSample3dMap->second.mFmodChannel->setFrequency(static_cast<float>(clampedPlayBackRate));
		}
		else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
		{
#ifdef _DEBUG
			fileName = iterSampleIdToSampleStreamMap->second.getPath();
#endif
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodStream);
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodChannel);

			iterSampleIdToSampleStreamMap->second.mFmodChannel->setFrequency(static_cast<float>(clampedPlayBackRate));
		}
		else
		{
			DEBUG_WARNING(true, ("Trying to set the playback rate of a sample [%i] that no longer exists.", sampleId.getId()));
		}

#ifdef _DEBUG
		DEBUG_WARNING(fileName && (finalPlayBackRate < minPlayBackRate || finalPlayBackRate > maxPlayBackRate), ("Audio::setSamplePlayBackRate(%s): Invalid playbackrate specified: %d (rate=%d, delta=%1.2f) clamping to (min=%d, max=%d)", fileName->getString(), finalPlayBackRate, playBackRate, playBackRateDelta, minPlayBackRate, maxPlayBackRate));
#endif
	}
}

//-----------------------------------------------------------------------------

float Audio::getSampleVolume(SampleId const &sampleId)
{
	float result = 0.0f;

	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);
		NOT_NULL(iterSampleIdToSample2dMap->second.mFmodChannel);

		iterSampleIdToSample2dMap->second.mFmodChannel->getVolume(&result);
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);
		NOT_NULL(iterSampleIdToSample3dMap->second.mFmodChannel);

		iterSampleIdToSample3dMap->second.mFmodChannel->getVolume(&result);
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodStream);
		NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodChannel);

		iterSampleIdToSampleStreamMap->second.mFmodChannel->getVolume(&result);
	}

	return result;
}

//-----------------------------------------------------------------------------

int Audio::getSamplePlayBackRate(SampleId const &sampleId)
{
	float result = 0.0f;

	if (s_installed)
	{
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		if (isSample2d(sampleId, iterSampleIdToSample2dMap))
		{
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodChannel);

			iterSampleIdToSample2dMap->second.mFmodChannel->getFrequency(&result);
		}
		else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
		{
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodChannel);

			iterSampleIdToSample3dMap->second.mFmodChannel->getFrequency(&result);
		}
		else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
		{
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodStream);
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodChannel);

			iterSampleIdToSampleStreamMap->second.mFmodChannel->getFrequency(&result);
		}
		else
		{
			DEBUG_WARNING(true, ("Trying to get the playback rate of a sample [%i] that no longer exists.", sampleId.getId()));
		}
	}

	return static_cast<int>(result);
}

//-----------------------------------------------------------------------------

void stopSample(Sound2 const &sound)
{
	if (s_installed)
	{
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		if (isSample2d(sound.getSampleId(), iterSampleIdToSample2dMap))
		{
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample2dMap->second.mFmodChannel);

			iterSampleIdToSample2dMap->second.mFmodChannel->stop();
			iterSampleIdToSample2dMap->second.mFmodChannel = nullptr;
		}
		else if (isSample3d(sound.getSampleId(), iterSampleIdToSample3dMap))
		{
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodSample);
			NOT_NULL(iterSampleIdToSample3dMap->second.mFmodChannel);

			iterSampleIdToSample3dMap->second.mFmodChannel->stop();
			iterSampleIdToSample3dMap->second.mFmodChannel = nullptr;
		}
		else if (isSampleStream(sound.getSampleId(), iterSampleIdToSampleStreamMap))
		{
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodStream);
			NOT_NULL(iterSampleIdToSampleStreamMap->second.mFmodChannel);

			iterSampleIdToSampleStreamMap->second.mFmodChannel->stop();
			iterSampleIdToSampleStreamMap->second.mFmodChannel = nullptr;
			iterSampleIdToSampleStreamMap->second.mFmodStream = nullptr;
		}
		else
		{
			DEBUG_WARNING(true, ("Audio::stopSample - Trying to stop a sample [%i] that no longer exists.", sound.getSampleId().getId()));
		}
	}
}

//-----------------------------------------------------------------------------
void Audio::releaseSampleId(Sound2 const &sound)
{
	if (s_installed)
	{
		// Make sure the sample is stopped
		stopSample(sound);

		// Now release the id
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		if (isSample2d(sound.getSampleId(), iterSampleIdToSample2dMap))
		{
			iterSampleIdToSample2dMap->second.mFmodSample->release();
			--s_allocated2dSampleHandles;
			iterSampleIdToSample2dMap->second.mFmodSample = nullptr;
			iterSampleIdToSample2dMap->second.mFmodChannel = nullptr;

			s_sampleIdToSample2dMap.erase(iterSampleIdToSample2dMap);
			removeSoundFromPrioritizedPlayingSounds(sound);
		}
		else if (isSample3d(sound.getSampleId(), iterSampleIdToSample3dMap))
		{
			iterSampleIdToSample3dMap->second.mFmodSample->release();
			--s_allocated3dSampleHandles;
			iterSampleIdToSample3dMap->second.mFmodSample = nullptr;
			iterSampleIdToSample3dMap->second.mFmodChannel = nullptr;

			s_sampleIdToSample3dMap.erase(iterSampleIdToSample3dMap);
			removeSoundFromPrioritizedPlayingSounds(sound);
		}
		else if (isSampleStream(sound.getSampleId(), iterSampleIdToSampleStreamMap))
		{
			iterSampleIdToSampleStreamMap->second.mFmodStream->release();
			iterSampleIdToSampleStreamMap->second.mFmodStream = nullptr;
			iterSampleIdToSampleStreamMap->second.mFmodChannel = nullptr;
			
			s_sampleIdToSampleStreamMap.erase(iterSampleIdToSampleStreamMap);
			removeSoundFromPrioritizedPlayingSounds(sound);
		}
		else
		{
			DEBUG_WARNING(true, ("Trying to release a sample [%i] that no longer exists.", sound.getSampleId().getId()));
		}
	}
}

//-----------------------------------------------------------------------------

float Audio::getSoundTemplateVolume(SoundId const &soundId)
{
	float result = 0.0f;
	const auto it = getIterSoundIdToSoundMap(soundId);

	if (it != s_soundIdToSoundMap.end())
	{
		result = it->second->getTemplateVolume();
	}

	return result;
}

//-----------------------------------------------------------------------------

float Audio::getSoundAttenuation(SoundId const &soundId)
{
	float result = 0.0f;
	const auto it = getIterSoundIdToSoundMap(soundId);

	if (it != s_soundIdToSoundMap.end())
	{
		result = it->second->getAttenuation();
	}

	return result;
}

//-----------------------------------------------------------------------------

float Audio::getSoundVolume(SoundId const &soundId)
{
	float result = 0.0f;
	auto it = getIterSoundIdToSoundMap(soundId);

	if (it != s_soundIdToSoundMap.end())
	{
		result = it->second->getVolume();
	}

	return result;
}

// Adjustment in half steps
//-----------------------------------------------------------------------------
float Audio::getSoundPitchDelta(SoundId const &soundId)
{
	float result = 0.0f;
	const auto it = getIterSoundIdToSoundMap(soundId);

	if (it != s_soundIdToSoundMap.end())
	{
		result = it->second->getPitchDelta();
	}

	return result;
}

//-----------------------------------------------------------------------------

int Audio::getCachedSampleCount()
{
	return static_cast<int>(s_sampleCache.size());
}

//-----------------------------------------------------------------------------


int Audio::getSample2dCount()
{
	return static_cast<int>(s_sampleIdToSample2dMap.size());
}

//-----------------------------------------------------------------------------

int Audio::getSample3dCount()
{
	return static_cast<int>(s_sampleIdToSample3dMap.size());
}

//-----------------------------------------------------------------------------

int Audio::getSampleStreamCount()
{
	return static_cast<int>(s_sampleIdToSampleStreamMap.size());
}

//-----------------------------------------------------------------------------

int Audio::getSampleCount()
{
	return getSample2dCount() + getSample3dCount() + getSampleStreamCount();
}

//-----------------------------------------------------------------------------

int Audio::getSoundCount()
{
	return static_cast<int>(s_soundIdToSoundMap.size());
}

//-----------------------------------------------------------------------------

float Audio::getDigitalCpuPercent()
{
	float result = 0.0f;

	if (s_installed)
	{
		s_fmod_core_system->getCPUUsage(nullptr, nullptr, nullptr, nullptr, &result);
	}

	return result;
}

//-----------------------------------------------------------------------------

int Audio::getDigitalLatency()
{
	int result = 0;

	if (s_installed)
	{
		// nope
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId Audio::playSound(Iff &iff)
{
	return playSound(iff, nullptr, nullptr);
}

//-----------------------------------------------------------------------------

SoundId Audio::playSound(Iff &iff, Vector const &position)
{
	return playSound(iff, &position, nullptr);
}

//-----------------------------------------------------------------------------

SoundId Audio::playSound(Iff &iff, Vector const * const position, CellProperty const * const parentCell)
{
	SoundId result;

	if (s_installed)
	{
		SoundTemplate const *soundTemplate = SoundTemplateList::fetch(&iff);

		if (soundTemplate != nullptr)
		{
			if (position != nullptr && soundTemplate->is3d())
			{
				result = playSound3d(soundTemplate, *position, parentCell);
			}
			else if (soundTemplate->is2d())
			{
				result = playSound2d(soundTemplate, position, parentCell);
			}
			else
			{
				DEBUG_WARNING(true, ("Audio::playSound - Attempting to play a sound with an invalid iff file: %s", iff.getFileName () ? iff.getFileName () : "null"));
			}

			SoundTemplateList::release(soundTemplate);
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

SoundId Audio::attachSound(char const *path, Object const *object, char const *hardPointName)
{
	return AudioNamespace::playSound(path, nullptr, object, hardPointName, nullptr);
}

//-----------------------------------------------------------------------------

SoundId Audio::attachSound(Iff &iff, Object const *object, char const *hardPointName)
{
	SoundId result;

	if (s_installed && object != nullptr)
	{
		SoundTemplate const *soundTemplate = SoundTemplateList::fetch(&iff);

		if (soundTemplate != nullptr)
		{
			result = ::attachSound(soundTemplate, object, hardPointName);

			SoundTemplateList::release(soundTemplate);
		}
	}

	return result;
}

// Removes the single sound from the object it is stuck to
//-----------------------------------------------------------------------------
void Audio::detachSound(SoundId const &soundId, float const fadeOutTime)
{
	stopSound(soundId, fadeOutTime);
}

// Removes all the sounds attached to the object
//-----------------------------------------------------------------------------
void Audio::detachSound(Object const &object, float const fadeOutTime)
{
	s_localPurgeList.clear();

	for(auto& iterSoundIdToSoundMap : s_soundIdToSoundMap)
	{
		if (iterSoundIdToSoundMap.second->getObject() == &object)
		{
			s_localPurgeList.emplace_back(iterSoundIdToSoundMap.first);
		}
	}

	// Stop all the sounds
	for(auto& i : s_localPurgeList)
	{
		stopSound(i, fadeOutTime);
	}
}

//-----------------------------------------------------------------------------

bool isSample2d(SampleId const &sampleId, SampleIdToSample2dMap::iterator &iterSampleIdToSample2dMap)
{
	bool result = false;

	if (s_installed)
	{
		iterSampleIdToSample2dMap = s_sampleIdToSample2dMap.find(sampleId);

		result = iterSampleIdToSample2dMap != s_sampleIdToSample2dMap.end();
	}

	return result;
}

//-----------------------------------------------------------------------------

bool isSample3d(SampleId const &sampleId, SampleIdToSample3dMap::iterator &iterSampleIdToSample3dMap)
{
	bool result = false;

	if (s_installed)
	{
		iterSampleIdToSample3dMap = s_sampleIdToSample3dMap.find(sampleId);

		result = iterSampleIdToSample3dMap != s_sampleIdToSample3dMap.end();
	}

	return result;
}

//-----------------------------------------------------------------------------

bool isSampleStream(SampleId const &sampleId, SampleIdToSampleStreamMap::iterator &iterSampleIdToSampleStreamMap)
{
	bool result = false;

	if (s_installed)
	{
		iterSampleIdToSampleStreamMap = s_sampleIdToSampleStreamMap.find(sampleId);

		result = iterSampleIdToSampleStreamMap != s_sampleIdToSampleStreamMap.end();
	}

	return result;
}


//-----------------------------------------------------------------------------
// only used by soundeditor
AudioSampleInformation Audio::getSampleInformation(std::string const &path)
{
	AudioSampleInformation audioSampleInformation;

	/*if (s_installed)
	{
		if (!path.empty())
		{
			AILSOUNDINFO soundInfo;

			AbstractFile *file = TreeFile::open(path.c_str(), AbstractFile::PriorityData, true);
			if (file)
			{
				int fileSize = file->length();
				byte *fileImage = file->readEntireFileAndClose();
				delete file;

				S32 result = false;//AIL_WAV_info(fileImage, &soundInfo);

				if (result)
				{
					audioSampleInformation.m_bits = static_cast<int>(soundInfo.bits);
					audioSampleInformation.m_blockSize = static_cast<int>(soundInfo.block_size);
					audioSampleInformation.m_channels = static_cast<int>(soundInfo.channels);
					audioSampleInformation.m_dataLength = static_cast<int>(soundInfo.data_len);
					audioSampleInformation.m_format = static_cast<int>(soundInfo.format);

					float timeCurrent = 0;
					::getSampleTime(path.c_str(), fileImage, fileSize, audioSampleInformation.m_time, timeCurrent);

					audioSampleInformation.m_rate = static_cast<int>(soundInfo.rate);
					audioSampleInformation.m_samples = static_cast<int>(soundInfo.samples);
					audioSampleInformation.m_type = getSampleType(fileImage, fileSize);
				}

				delete [] fileImage;
			}
		}
	}*/

	return audioSampleInformation;
}

// End all sounds and invalidates their sound ids
//-----------------------------------------------------------------------------
void Audio::stopAllSounds(float const fadeOutTime)
{
	bool const keepAlive = false;
	::stopAllSounds(fadeOutTime, keepAlive);

	DEBUG_FATAL(!s_soundIdToSoundMap.empty() && (fadeOutTime <= 0.0f), ("Audio::stopAllSounds - s_soundIdToSoundMap not empty"));
}

//-----------------------------------------------------------------------------

void stopAllSounds(float const fadeOutTime, bool const keepAlive)
{
	s_localPurgeList.clear();

	for(const auto& iterSoundIdToSoundMap : s_soundIdToSoundMap)
	{
		s_localPurgeList.emplace_back(iterSoundIdToSoundMap.first);
	}

	for(const auto& soundId : s_localPurgeList)
	{
		stopSound(soundId, fadeOutTime, keepAlive);
	}

	s_localPurgeList.clear();

	Audio::alter(0.0f, s_listenerObject);
}

//-----------------------------------------------------------------------------

std::string const &Audio::getCurrent3dProvider()
{
	return s_soundProvider;
}


// Set a filter that affects all sounds, but not music
//-----------------------------------------------------------------------------
void Audio::setRoomType(RoomType const roomType)
{
	if (s_installed && isRoomTypeSupported())
	{
		FMOD_REVERB_PROPERTIES revProps = FMOD_PRESET_GENERIC;
		
		switch (roomType)
		{
			case RT_alley:           { revProps = FMOD_PRESET_ALLEY; } break;
			case RT_arena:           { revProps = FMOD_PRESET_ARENA; } break;
			case RT_auditorium:      { revProps = FMOD_PRESET_AUDITORIUM; } break;
			case RT_bathRoom:        { revProps = FMOD_PRESET_BATHROOM; } break;
			case RT_carpetedHallway: { revProps = FMOD_PRESET_CARPETTEDHALLWAY; } break;
			case RT_cave:            { revProps = FMOD_PRESET_CAVE; } break;
			case RT_city:            { revProps = FMOD_PRESET_CITY; } break;
			case RT_concertHall:     { revProps = FMOD_PRESET_CONCERTHALL; } break;
			case RT_dizzy:           { revProps = FMOD_PRESET_GENERIC; } break;
			case RT_drugged:         { revProps = FMOD_PRESET_GENERIC; } break;
			case RT_forest:          { revProps = FMOD_PRESET_FOREST; } break;
			case RT_generic:         { revProps = FMOD_PRESET_GENERIC; } break;
			case RT_hallway:         { revProps = FMOD_PRESET_HALLWAY; } break;
			case RT_hangar:          { revProps = FMOD_PRESET_HANGAR; } break;
			case RT_livingRoom:      { revProps = FMOD_PRESET_LIVINGROOM; } break;
			case RT_mountains:       { revProps = FMOD_PRESET_MOUNTAINS; } break;
			case RT_paddedCell:      { revProps = FMOD_PRESET_PADDEDCELL; } break;
			case RT_parkingLot:      { revProps = FMOD_PRESET_PARKINGLOT; } break;
			case RT_plain:           { revProps = FMOD_PRESET_PLAIN; } break;
			case RT_psychotic:       { revProps = FMOD_PRESET_GENERIC; } break;
			case RT_quarry:          { revProps = FMOD_PRESET_QUARRY; } break;
			case RT_room:            { revProps = FMOD_PRESET_ROOM; } break;
			case RT_sewerPipe:       { revProps = FMOD_PRESET_SEWERPIPE; } break;
			case RT_stoneCorridor:   { revProps = FMOD_PRESET_STONECORRIDOR; } break;
			case RT_stoneRoom:       { revProps = FMOD_PRESET_STONEROOM; } break;
			case RT_underWater:      { revProps = FMOD_PRESET_UNDERWATER; } break;
			default:                 { DEBUG_FATAL(true, ("Trying to set an unknown room type.")); } break;
		}

		s_fmod_core_system->setReverbProperties(0, &revProps);
	}
}

//-----------------------------------------------------------------------------

Audio::RoomType Audio::getRoomType()
{
	RoomType result = RT_notSupported;

	if (s_fmod_core_system != nullptr)
	{
		FMOD_REVERB_PROPERTIES revProps = { 0 };
		s_fmod_core_system->getReverbProperties(0, &revProps);

		if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_ALLEY)) result = RT_alley;
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_ARENA)) result = RT_arena;
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_AUDITORIUM)) result = RT_auditorium;
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_BATHROOM)) result = RT_bathRoom;			
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_CARPETTEDHALLWAY)) result = RT_carpetedHallway;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_CAVE)) result = RT_cave;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_CITY)) result = RT_city;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_CONCERTHALL)) result = RT_concertHall;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_GENERIC)) result = RT_generic;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_FOREST)) result = RT_forest;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_HALLWAY)) result = RT_hallway;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_HANGAR)) result = RT_hangar;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_LIVINGROOM)) result = RT_livingRoom;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_MOUNTAINS)) result = RT_mountains;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_PADDEDCELL)) result = RT_paddedCell;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_PARKINGLOT)) result = RT_parkingLot;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_PLAIN)) result = RT_plain;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_QUARRY)) result = RT_quarry;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_ROOM)) result = RT_room;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_SEWERPIPE)) result = RT_sewerPipe;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_STONECORRIDOR)) result = RT_stoneCorridor;
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_STONEROOM)) result = RT_stoneRoom;	
		else if(revProps == FMOD_REVERB_PROPERTIES(FMOD_PRESET_UNDERWATER)) result = RT_underWater;	
	}

	return result;
}

//-----------------------------------------------------------------------------

char const * const Audio::getRoomTypeString()
{
	static char const * result = "";

	switch (getRoomType())
	{
		case RT_alley:           { result = "Alley"; } break;
		case RT_arena:           { result = "Arena"; } break;
		case RT_auditorium:      { result = "Auditorium"; } break;
		case RT_bathRoom:        { result = "Bathroom"; } break;
		case RT_carpetedHallway: { result = "Carpeted hallway"; } break;
		case RT_cave:            { result = "Cave"; } break;
		case RT_city:            { result = "City"; } break;
		case RT_concertHall:     { result = "Concert hall"; } break;
		case RT_dizzy:           { result = "Dizzy"; } break;
		case RT_drugged:         { result = "Drugged"; } break;
		case RT_forest:          { result = "Forest"; } break;
		case RT_generic:         { result = "Generic"; } break;
		case RT_hallway:         { result = "Hallway"; } break;
		case RT_hangar:          { result = "Hangar"; } break;
		case RT_livingRoom:      { result = "Living room"; } break;
		case RT_mountains:       { result = "Mountains"; } break;
		case RT_paddedCell:      { result = "Padded cell"; } break;
		case RT_parkingLot:      { result = "Parking lot"; } break;
		case RT_plain:           { result = "Plain"; } break;
		case RT_psychotic:       { result = "Psychotic"; } break;
		case RT_quarry:          { result = "Quarry"; } break;
		case RT_room:            { result = "Room"; } break;
		case RT_sewerPipe:       { result = "Sewer pipe"; } break;
		case RT_stoneCorridor:   { result = "Stone corridor"; } break;
		case RT_stoneRoom:       { result = "Stone room"; } break;
		case RT_underWater:      { result = "Underwater"; } break;
		default:                 { result = "Not Supported"; } break;
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::setSampleEffectsLevel(SampleId const &sampleId, float const effectLevel)
{
	DEBUG_FATAL(((effectLevel == -0.0f) || (effectLevel < 0.0f) || (effectLevel > 1.0f)), ("effectLevel out of range: %f", effectLevel));

	if (s_installed)
	{
		auto it = getIterSampleIdToSample3dMap(sampleId);

		if (it != s_sampleIdToSample3dMap.end())
		{
			//NOT_NULL(it->second.mFmodSample);
			//NOT_NULL(it->second.mFmodChannel);

			// TODO : dry level ??
			it->second.mFmodChannel->setReverbProperties(0, 0.0f);
		}
	}
}

//-----------------------------------------------------------------------------

float Audio::getSampleEffectsLevel(SampleId const &sampleId)
{
	float resultDry = 0.0f;
	float resultWet = 0.0f;

	if (s_installed)
	{
		auto it = getIterSampleIdToSample3dMap(sampleId);

		if (it != s_sampleIdToSample3dMap.end())
		{
			NOT_NULL(it->second.mFmodSample);
			NOT_NULL(it->second.mFmodChannel);

			// TODO : dry level ?
			it->second.mFmodChannel->getReverbProperties(0, &resultWet);
		}
	}

	return resultDry;
}

//-----------------------------------------------------------------------------

static int once = true;

FMOD_RESULT F_CALLBACK fileOpenCallBack(const char *fileName, unsigned int *fileSize, void **fileHandle, void *userData)
{
	if (!Os::isMainThread() && !PerThreadData::isThreadInstalled())
	{
		once = false;
		PerThreadData::threadInstall(false);
	}

	AbstractFile *abstractFile = TreeFile::open(fileName, AbstractFile::PriorityAudioVideo, true);

	if (abstractFile != nullptr)
	{
		*fileSize = static_cast<unsigned int>(abstractFile->length());
		*fileHandle = reinterpret_cast<void *>(s_nextFileHandle);
		s_fileMap.insert(std::make_pair(s_nextFileHandle, abstractFile));
		++s_nextFileHandle;

#ifdef _DEBUG
		ms_handleNameMap.insert(std::make_pair(reinterpret_cast<uintptr_t>(*fileHandle), fileName));
#endif
	}
	else
	{
		*fileHandle = nullptr;
	}

	return (abstractFile != nullptr ? FMOD_OK : FMOD_ERR_FILE_NOTFOUND);
}

//-----------------------------------------------------------------------------

FMOD_RESULT F_CALLBACK fileCloseCallBack(void *fileHandle, void *userData)
{
	if (!Os::isMainThread() && !PerThreadData::isThreadInstalled())
	{
		once = false;
		PerThreadData::threadInstall(false);
	}

	auto fileMapIter = s_fileMap.find(reinterpret_cast<uintptr_t>(fileHandle));

	if (fileMapIter != s_fileMap.end())
	{
		AbstractFile *abstractFile = fileMapIter->second;

		abstractFile->close();

		delete abstractFile;

		s_fileMap.erase(fileMapIter);

#ifdef _DEBUG
		ms_fileCloseHandleSet.insert(reinterpret_cast<uintptr_t>(fileHandle));
#endif
	}
	else
	{
#ifdef _DEBUG
		determineCallbackError("close", reinterpret_cast<uintptr_t>(fileHandle));
#endif
		return FMOD_ERR_FILE_NOTFOUND;
	}

	return FMOD_OK;
}

//-----------------------------------------------------------------------------

static FMOD_RESULT F_CALLBACK fileSeekCallBack(void *fileHandle, unsigned int offset, void *userData)
{
	if (!Os::isMainThread() && !PerThreadData::isThreadInstalled())
	{
		once = false;
		PerThreadData::threadInstall(false);
	}

	auto fileMapIter = s_fileMap.find(reinterpret_cast<uintptr_t>(fileHandle));

	if (fileMapIter != s_fileMap.end())
	{
		AbstractFile *abstractFile = fileMapIter->second;

		abstractFile->seek(AbstractFile::SeekBegin, static_cast<int>(offset));
		
		// Set the current file position
		abstractFile->tell();
	}
	else
	{
#ifdef _DEBUG
		determineCallbackError("seek", reinterpret_cast<uintptr_t>(fileHandle));
#endif
		return FMOD_ERR_FILE_COULDNOTSEEK;
	}

	return FMOD_OK;
}

//-----------------------------------------------------------------------------

static FMOD_RESULT F_CALLBACK fileReadCallBack(void *fileHandle, void *buffer, unsigned int sizeBytes, unsigned int *bytesRead, void *userData)
{
	if (!Os::isMainThread() && !PerThreadData::isThreadInstalled())
	{
		once = false;
		PerThreadData::threadInstall(false);
	}

	auto fileMapIter = s_fileMap.find(reinterpret_cast<uintptr_t>(fileHandle));

	if (fileMapIter != s_fileMap.end())
	{
		AbstractFile *abstractFile = fileMapIter->second;
		
		*bytesRead = static_cast<unsigned int>(abstractFile->read(buffer, static_cast<int>(sizeBytes)));
	}
	else
	{
#ifdef _DEBUG
		determineCallbackError("read", reinterpret_cast<uintptr_t>(fileHandle));
#endif
		return FMOD_ERR_FILE_BAD;
	}

	return FMOD_OK;
}

//-----------------------------------------------------------------------------

bool Audio::isRoomTypeSupported()
{
	return (getRoomType() != RT_notSupported);
}

//-----------------------------------------------------------------------------
// Gets the total and current milliseconds of the current sample in the sound
// that is playing.
//-----------------------------------------------------------------------------
void Audio::getCurrentSampleTime(SoundId const &soundId, float &timeTotal, float &timeCurrent)
{
	timeTotal = 0.0f;
	timeCurrent = 0.0f;

	auto iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 *sound = iterSoundMap->second;

		if (sound->getSampleId().getId() != 0)
		{
			getSampleTime(sound->getSampleId(), timeTotal, timeCurrent);
		}
	}
}

//-----------------------------------------------------------------------------

void Audio::setCurrentSoundTime(SoundId const &soundId, int const milliSecond)
{
	auto iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 *sound = iterSoundMap->second;

		if (sound != nullptr)
		{
			sound->setCurrentTime(milliSecond);
		}
	}
}

//-----------------------------------------------------------------------------

bool Audio::getCurrentSoundTime(SoundId const &soundId, int &milliSecond)
{
	bool result = false;
	const SoundIdToSoundMap::iterator iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 * const sound = iterSoundMap->second;

		if (sound != nullptr)
		{
			result = true;
			milliSecond = sound->getCurrentTime();
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool Audio::getCurrentSoundTime(SoundId const &soundId, int &totalMilliSecond, int &milliSecond)
{
	bool result = false;
	const SoundIdToSoundMap::iterator iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 * const sound = iterSoundMap->second;

		if (sound != nullptr)
		{
			result = true;
			milliSecond = sound->getCurrentTime();
			totalMilliSecond = sound->getTotalTime ();
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool Audio::getTotalSoundTime(SoundId const &soundId, int &milliSecond)
{
	bool result = false;
	const SoundIdToSoundMap::iterator iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 * const sound = iterSoundMap->second;

		if (sound != nullptr)
		{
			result = true;
			milliSecond = sound->getTotalTime();
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool Audio::isSampleAtEnd(SoundId const &soundId)
{
	bool result = false;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	auto iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		Sound2 *sound = iterSoundMap->second;

		if (isSampleStream(sound->getSampleId(), iterSampleIdToSampleStreamMap))
		{
			unsigned int millisecondsTotal = 0;
			unsigned int millisecondsCurrent = 0;

			iterSampleIdToSampleStreamMap->second.mFmodChannel->getPosition(&millisecondsCurrent, FMOD_TIMEUNIT_MS);
			iterSampleIdToSampleStreamMap->second.mFmodStream->getLength(&millisecondsTotal, FMOD_TIMEUNIT_MS);
			
			result = millisecondsTotal == millisecondsCurrent;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::getSampleTime(SampleId const &sampleId, float &timeTotal, float &timeCurrent)
{
	timeTotal = 0.0f;
	timeCurrent = 0.0f;

	if (s_installed)
	{
		SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
		SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
		SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

		if (isSample2d(sampleId, iterSampleIdToSample2dMap))
		{
			unsigned int millisecondsTotal = 0;
			unsigned int millisecondsCurrent = 0;

			iterSampleIdToSample2dMap->second.mFmodChannel->getPosition(&millisecondsCurrent, FMOD_TIMEUNIT_MS);
			iterSampleIdToSample2dMap->second.mFmodSample->getLength(&millisecondsTotal, FMOD_TIMEUNIT_MS);
			
			timeTotal = static_cast<float>(millisecondsTotal) / 1000.0f;
			timeCurrent = static_cast<float>(millisecondsCurrent) / 1000.0f;
		}
		else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
		{
			timeCurrent = 0.0f;

			SampleCache::const_iterator iterSampleCache = s_sampleCache.find(iterSampleIdToSample3dMap->second.getPath());
			DEBUG_WARNING((iterSampleCache == s_sampleCache.end()), ("Error finding the sample in the cache: %s", iterSampleIdToSample3dMap->second.getPath()->getString()));

			if (iterSampleCache != s_sampleCache.end())
			{
				timeTotal = iterSampleCache->second.m_time;

				unsigned int sampleOffset = 0;
				iterSampleIdToSample3dMap->second.mFmodChannel->getPosition(&sampleOffset, FMOD_TIMEUNIT_PCMBYTES);
				
				int const sampleSize = iterSampleIdToSample3dMap->second.getFileSize();

				if (sampleSize > 0)
				{
					timeCurrent = timeTotal * (static_cast<float>(sampleOffset) / static_cast<float>(sampleSize));
				}
			}
		}
		else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
		{
			unsigned int millisecondsTotal = 0;
			unsigned int millisecondsCurrent = 0;

			iterSampleIdToSampleStreamMap->second.mFmodChannel->getPosition(&millisecondsCurrent, FMOD_TIMEUNIT_MS);
			iterSampleIdToSampleStreamMap->second.mFmodStream->getLength(&millisecondsTotal, FMOD_TIMEUNIT_MS);
			
			timeTotal = static_cast<float>(millisecondsTotal) / 1000.0f;
			timeCurrent = static_cast<float>(millisecondsCurrent) / 1000.0f;
		}
	}
}

//-----------------------------------------------------------------------------

int Audio::getSampleCurrentTime(SampleId const &sampleId)
{
	unsigned int result = 0;
	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		iterSampleIdToSample2dMap->second.mFmodChannel->getPosition(&result, FMOD_TIMEUNIT_MS);
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		SampleCache::const_iterator iterSampleCache = s_sampleCache.find(iterSampleIdToSample3dMap->second.getPath());
		DEBUG_WARNING((iterSampleCache == s_sampleCache.end()), ("Error finding the sample in the cache: %s", iterSampleIdToSample3dMap->second.getPath()->getString()));

		if (iterSampleCache != s_sampleCache.end())
		{
			unsigned int sampleOffset = 0;
			iterSampleIdToSample3dMap->second.mFmodChannel->getPosition(&sampleOffset, FMOD_TIMEUNIT_PCMBYTES);
			
			int const sampleSize = iterSampleIdToSample3dMap->second.getFileSize();

			if (sampleSize > 0)
			{
				result = static_cast<int>(iterSampleCache->second.m_time * (static_cast<float>(sampleOffset) / static_cast<float>(sampleSize)) * 1000.0f);
			}
		}
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		iterSampleIdToSampleStreamMap->second.mFmodChannel->getPosition(&result, FMOD_TIMEUNIT_MS);
	}

	return static_cast<unsigned int>(result);
}

//-----------------------------------------------------------------------------

unsigned int Audio::getSampleTotalTime(SampleId const &sampleId)
{
	unsigned int result = 0;
	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		unsigned int millisecondsTotal = 0;

		iterSampleIdToSample2dMap->second.mFmodSample->getLength(&millisecondsTotal, FMOD_TIMEUNIT_MS);

		result = millisecondsTotal;
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		SampleCache::const_iterator iterSampleCache = s_sampleCache.find(iterSampleIdToSample3dMap->second.getPath());
		DEBUG_WARNING((iterSampleCache == s_sampleCache.end()), ("Error finding the sample in the cache: %s", iterSampleIdToSample3dMap->second.getPath()->getString()));

		if (iterSampleCache != s_sampleCache.end())
		{
			result = static_cast<unsigned int>(iterSampleCache->second.m_time * 1000.0f);
		}
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		unsigned int millisecondsTotal = 0;

		iterSampleIdToSampleStreamMap->second.mFmodStream->getLength(&millisecondsTotal, FMOD_TIMEUNIT_MS);

		result = millisecondsTotal;
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::setSampleCurrentTime(SampleId const &sampleId, int const milliSeconds)
{
	SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
	SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
	SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

	if (isSample2d(sampleId, iterSampleIdToSample2dMap))
	{
		if (milliSeconds == 0)
		{
			FMOD_RESULT fr = s_fmod_core_system->playSound(iterSampleIdToSample2dMap->second.mFmodSample, nullptr, false, &iterSampleIdToSample2dMap->second.mFmodChannel);

			if(fr != FMOD_OK)
			{
				REPORT_LOG(true, ("Audio: setsamplecurrenttime: failed playback\n"));
			}
		}
		else
		{
			if(iterSampleIdToSample2dMap->second.mFmodChannel)
			{
				iterSampleIdToSample2dMap->second.mFmodChannel->setPosition(milliSeconds, FMOD_TIMEUNIT_MS);
			}
		}
	}
	else if (isSample3d(sampleId, iterSampleIdToSample3dMap))
	{
		int const totalTime = getSampleTotalTime(sampleId);

		if (totalTime > 0)
		{
			float const percent = static_cast<float>(milliSeconds) / static_cast<float>(totalTime);
			float const sampleSize = static_cast<float>(getSampleSize(iterSampleIdToSample3dMap->second.getPath()->getString()));
			unsigned int offset = static_cast<unsigned int>(sampleSize * percent);

			offset -= offset % 4;

			if(iterSampleIdToSample3dMap->second.mFmodChannel)
			{
				iterSampleIdToSample3dMap->second.mFmodChannel->setPosition(offset, FMOD_TIMEUNIT_PCMBYTES);
			}
		}
		else
		{
			DEBUG_REPORT_LOG(true, ("Audio::setSampleCurrentTime() - Unable to set the requested 3d sample time.\n"));
		}
	}
	else if (isSampleStream(sampleId, iterSampleIdToSampleStreamMap))
	{
		if (milliSeconds == 0)
		{
			FMOD_RESULT fr = s_fmod_core_system->playSound(iterSampleIdToSampleStreamMap->second.mFmodStream, nullptr, false, &iterSampleIdToSampleStreamMap->second.mFmodChannel);

			if(fr != FMOD_OK)
			{
				REPORT_LOG(true, ("Audio: setsamplecurrenttime: failed playback\n"));
			}
		}
		else
		{
			if(iterSampleIdToSampleStreamMap->second.mFmodChannel)
			{
				iterSampleIdToSampleStreamMap->second.mFmodChannel->setPosition(milliSeconds, FMOD_TIMEUNIT_MS);
			}
		}
	}
}

//-----------------------------------------------------------------------------

void getSampleTime(char const *path, byte *fileImage, int fileSize, float &timeTotal, float &timeCurrent)
{
	timeTotal = 0.0f;
	timeCurrent = 0.0f;

	if (s_installed && (fileSize > 0))
	{
		unsigned int total = 0;
		
		FMOD::Sound* sample = nullptr;
		
		FMOD_CREATESOUNDEXINFO info;
		memset(&info, 0, sizeof(info));
		info.length = fileSize;
		info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

		FMOD_RESULT fr = s_fmod_core_system->createSound(reinterpret_cast<const char*>(fileImage), FMOD_OPENMEMORY, &info, &sample);

		if (fr == FMOD_OK)
		{
			SampleCacheEntry sampleCacheEntry;
			sampleCacheEntry.m_sampleRawData = fileImage;
			sampleCacheEntry.m_fileSize = fileSize;
			sampleCacheEntry.setExtension(path);

			if (sampleCacheEntry.m_sampleRawData != nullptr)
			{
				sample->getLength(&total, FMOD_TIMEUNIT_MS);
			}

			sample->release();
		}

		timeTotal = static_cast<float>(total) / 1000.0f;
		timeCurrent = 0.0f;
	}
}

//-----------------------------------------------------------------------------

void Audio::getCurrentSample(SoundId const &soundId, TemporaryCrcString &path)
{
	path.clear();

	if (s_installed)
	{
		auto current = s_soundIdToSoundMap.find(soundId);

		if (current != s_soundIdToSoundMap.end())
		{
			Sound2 *sound = current->second;

			if (sound->getSampleId().getId() != 0)
			{
				SampleIdToSample2dMap::iterator iterSampleIdToSample2dMap;
				SampleIdToSample3dMap::iterator iterSampleIdToSample3dMap;
				SampleIdToSampleStreamMap::iterator iterSampleIdToSampleStreamMap;

				if (isSample2d(sound->getSampleId(), iterSampleIdToSample2dMap))
				{
					CrcString const &crcString = *iterSampleIdToSample2dMap->second.getPath();

					path.set(crcString.getString(), crcString.getCrc());
				}
				else if (isSample3d(sound->getSampleId(), iterSampleIdToSample3dMap))
				{
					CrcString const &crcString = *iterSampleIdToSample3dMap->second.getPath();

					path.set(crcString.getString(), crcString.getCrc());
				}
				else if (isSampleStream(sound->getSampleId(), iterSampleIdToSampleStreamMap))
				{
					CrcString const &crcString = *iterSampleIdToSampleStreamMap->second.getPath();

					path.set(crcString.getString(), crcString.getCrc());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

SoundIdToSoundMap::iterator getIterSoundIdToSoundMap(SoundId const &soundId)
{
#ifdef _DEBUG
	int const count = static_cast<int>(s_soundIdToSoundMap.size());
	UNREF(count);
#endif // _DEBUG

	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	DEBUG_FATAL(s_installed && (iterSoundIdToSoundMap == s_soundIdToSoundMap.end()), ("Audio::getIterSoundIdToSoundMap - Trying to get a sound id [%i] that no longer exists.", soundId.getId()));

	return iterSoundIdToSoundMap;
}

//-----------------------------------------------------------------------------

SampleIdToSample3dMap::iterator getIterSampleIdToSample3dMap(SampleId const &sampleId)
{
	auto iterSampleIdToSample3dMap = s_sampleIdToSample3dMap.find(sampleId);

	DEBUG_FATAL(s_installed && (iterSampleIdToSample3dMap == s_sampleIdToSample3dMap.end()), ("Audio::getIterSampleIdToSample3dMap - Trying to get a sample id [%i] of a 3d sample that no longer exists.", sampleId.getId()));

	return iterSampleIdToSample3dMap;
}

//-----------------------------------------------------------------------------
char const *getFileError()
{
	char const *result = "";

	return result;
}

//-----------------------------------------------------------------------------
Vector const &Audio::getListenerPosition()
{
	return s_listener.m_positionCurrent;
}

//-----------------------------------------------------------------------------

int getNextSampleId()
{
	// Find the next not used sample id
	do
	{
		++s_nextSampleId;

		// Handle the wrap
		if (s_nextSampleId >= std::numeric_limits<int>::max())
		{
			s_nextSampleId = 1;
		}
	}
	while (Audio::isSampleValid(SampleId(s_nextSampleId)));

	return s_nextSampleId;
}

//-----------------------------------------------------------------------------

int getNextSoundId()
{
	// Find the next not used sound id
	do
	{
		++s_nextSoundId;

		// Handle the wrap
		if (s_nextSoundId >= std::numeric_limits<int>::max())
		{
			s_nextSoundId = 1;
		}
	}
	while (Audio::isSoundValid(SoundId(s_nextSoundId, "")));

	return s_nextSoundId;
}

//-----------------------------------------------------------------------------

bool Audio::isSoundPlaying(SoundId const &soundId)
{
	return isSoundValid(soundId);
}

//-----------------------------------------------------------------------------

bool Audio::getSoundWorldPosition(SoundId const &soundId, Vector &position)
{
	bool result = false;

	SoundIdToSoundMap::const_iterator iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		position = iterSoundIdToSoundMap->second->getPosition_w();

		result = true;
	}

	return result;
}

//-----------------------------------------------------------------------------

float Audio::getFallOffDistance(float const minDistance)
{
	float result = minDistance;

	for (int j = 1; j < s_soundFallOffPower; ++j)
	{
		result *= 2;
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::setSoundFallOffPower(int const power)
{
	s_soundFallOffPower = clamp(3, power, 6);
}

//-----------------------------------------------------------------------------

bool Audio::getLoopOffsets(CrcString const &path, int &loopStartOffset, int &loopEndOffset)
{
	bool result = false;
	char const *fileName = path.getString();
	char const *currentPosition = fileName;
	bool done = false;

	do
	{
		currentPosition = strstr(fileName, "/");

		if (currentPosition != nullptr)
		{
			fileName = currentPosition + 1;
		}
		else
		{
			done = true;
		}
	}
	while (!done);

	TemporaryCrcString crcString(fileName, true);

	MusicOffsetMap::const_iterator iterMusicDataTableIndexMap = s_musicOffsetMap.find(&crcString);

	if (iterMusicDataTableIndexMap != s_musicOffsetMap.end())
	{
		result = true;

		int const row = iterMusicDataTableIndexMap->second;
		loopStartOffset = s_musicDataTable->getIntValue(1, row);
		loopEndOffset = s_musicDataTable->getIntValue(2, row);
	}
	else
	{
		loopStartOffset = 0;
		loopEndOffset = -1;
	}

	return result;
}

//-----------------------------------------------------------------------------

FMOD_RESULT F_CALLBACK endOfSample2dCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2)
{
	if(cType == FMOD_CHANNELCONTROL_CHANNEL && cbType == FMOD_CHANNELCONTROL_CALLBACK_END)
	{
		for(auto& iterSampleIdToSample2dMap : s_sampleIdToSample2dMap)
		{
			if (iterSampleIdToSample2dMap.second.mFmodChannel == reinterpret_cast<FMOD::Channel*>(chControl))
			{
				iterSampleIdToSample2dMap.second.m_status = Audio::PS_done;

				Sound2 *sound = iterSampleIdToSample2dMap.second.m_sound;
				NOT_NULL(sound);

				sound->endOfSample();

				break;
			}
		}
	}
	
	return FMOD_OK;
}

//-----------------------------------------------------------------------------

FMOD_RESULT F_CALLBACK endOfSample3dCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2)
{
	if(cType == FMOD_CHANNELCONTROL_CHANNEL && cbType == FMOD_CHANNELCONTROL_CALLBACK_END)
	{
		for(auto& iterSampleIdToSample3dMap : s_sampleIdToSample3dMap)
		{
			if(iterSampleIdToSample3dMap.second.mFmodChannel ==  reinterpret_cast<FMOD::Channel*>(chControl))
			{
				iterSampleIdToSample3dMap.second.m_status = Audio::PS_done;

				Sound2 *sound = iterSampleIdToSample3dMap.second.m_sound;
				NOT_NULL(sound);

				sound->endOfSample();

				break;
			}
		}
	}

	return FMOD_OK;
}

//-----------------------------------------------------------------------------

FMOD_RESULT F_CALLBACK endOfSampleStreamCallBack(FMOD_CHANNELCONTROL* chControl, FMOD_CHANNELCONTROL_TYPE cType, FMOD_CHANNELCONTROL_CALLBACK_TYPE cbType, void* cmdData1, void* cmdData2)
{
	if(cType == FMOD_CHANNELCONTROL_CHANNEL && cbType == FMOD_CHANNELCONTROL_CALLBACK_END)
	{
		for(auto& iterSampleIdToSampleStreamMap : s_sampleIdToSampleStreamMap)
		{
			if(iterSampleIdToSampleStreamMap.second.mFmodChannel == reinterpret_cast<FMOD::Channel*>(chControl))
			{
				iterSampleIdToSampleStreamMap.second.m_status = Audio::PS_done;

				Sound2 *sound = iterSampleIdToSampleStreamMap.second.m_sound;
				NOT_NULL(sound);

				sound->endOfSample();

				break;
			}
		}
	}

	return FMOD_OK;
}

//-----------------------------------------------------------------------------

void Audio::setMasterVolume(float const volume)
{
	s_masterVolume = clamp(0.0f, volume, 1.0f);
}

//-----------------------------------------------------------------------------

float Audio::getMasterVolume()
{
	return s_masterVolume;
}

//-----------------------------------------------------------------------------

void Audio::setSoundEffectVolume(float const volume)
{
	setSoundCategoryVolume(SC_explosion, volume);
	setSoundCategoryVolume(SC_item, volume);
	setSoundCategoryVolume(SC_vehicle, volume);
	setSoundCategoryVolume(SC_vocalization, volume);
	setSoundCategoryVolume(SC_weapon, volume);
	setSoundCategoryVolume(SC_movement, volume);
	setSoundCategoryVolume(SC_machine, volume);
	setSoundCategoryVolume(SC_installation, volume);
}

//-----------------------------------------------------------------------------

float Audio::getSoundEffectVolume()
{
	return getSoundCategoryVolume(SC_explosion, true);
}

//-----------------------------------------------------------------------------

float Audio::getAmbientEffectVolume()
{
	return getSoundCategoryVolume(SC_ambient, true);
}

//-----------------------------------------------------------------------------

void Audio::setAmbientEffectVolume(float volume)
{
	setSoundCategoryVolume(SC_ambient, volume);
}

//-----------------------------------------------------------------------------

void Audio::setBackGroundMusicVolume(float const volume)
{
	setSoundCategoryVolume(SC_backGroundMusic, volume);
}

//-----------------------------------------------------------------------------

float Audio::getBackGroundMusicVolume()
{
	return getSoundCategoryVolume(SC_backGroundMusic, true);
}

//-----------------------------------------------------------------------------

void Audio::setPlayerMusicVolume(float const volume)
{
	setSoundCategoryVolume(SC_playerMusic, volume);
}

//-----------------------------------------------------------------------------

float Audio::getPlayerMusicVolume()
{
	return getSoundCategoryVolume(SC_playerMusic, true);
}

//-----------------------------------------------------------------------------

void Audio::setUserInterfaceVolume(float const volume)
{
	setSoundCategoryVolume(SC_userInterface, volume);
}

//-----------------------------------------------------------------------------

float Audio::getUserInterfaceVolume()
{
	return getSoundCategoryVolume(SC_userInterface, true);
}

//-----------------------------------------------------------------------------

void Audio::setRequestedMaxNumberOfSamples(int const max)
{
	s_requestedMaxNumberOfSamples = clamp(16, max, 64);
}

//-----------------------------------------------------------------------------

int Audio::getRequestedMaxNumberOfSamples()
{
	return s_requestedMaxNumberOfSamples;
}

//-----------------------------------------------------------------------------

int Audio::getMaxDigitalMixerChannels()
{
	return s_maxDigitalMixerChannels;
}

//-----------------------------------------------------------------------------

void Audio::setMaxCached2dSampleSize(int const bytes)
{
	int const min = 1024 * 64;
	int const max = 1024 * 512;

	s_maxCached2dSampleSize = clamp(min, bytes, max);
}

//-----------------------------------------------------------------------------

int Audio::getMaxCached2dSampleSize()
{
	return s_maxCached2dSampleSize;
}

//-----------------------------------------------------------------------------

Object const * const Audio::getListener()
{
	return s_listenerObject.getPointer();
}

//-----------------------------------------------------------------------------

void Audio::lock()
{
	
}

//-----------------------------------------------------------------------------

void Audio::unLock()
{
	
}

//-----------------------------------------------------------------------------

void Audio::setAutoDelete(SoundId const &soundId, bool const autoDelete)
{
	auto iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		iterSoundMap->second->setAutoDelete(autoDelete);
	}
}

//-----------------------------------------------------------------------------

void Audio::serve()
{
	if (Os::isMainThread() && s_fmod_studio_system != nullptr && !s_prioritizedPlayingSounds.empty() && s_audioServePerformanceTimer != nullptr)
	{
		static float deltaTime = 0.0f;
		
		s_audioServePerformanceTimer->stop();
		deltaTime += s_audioServePerformanceTimer->getElapsedTime();
		s_audioServePerformanceTimer->start();

		if (deltaTime > 0.05f)
		{
			deltaTime = 0.0f;
			s_fmod_studio_system->update();
		}
	}
}

//-----------------------------------------------------------------------------

void Audio::setEndOfSampleCallBack(SoundId const &soundId, EndOfSampleCallBack callBack)
{
	auto iterSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundMap != s_soundIdToSoundMap.end())
	{
		iterSoundMap->second->setEndOfSampleCallBack(callBack);
	}
}

//----------------------------------------------------------------------

void Audio::getSoundsAttachedToObject (const Object & obj, SoundVector & sv)
{
	for(const auto& it : s_soundIdToSoundMap)
	{
		Sound2 * const sound = it.second;

		if (sound && sound->getObject () == &obj)
			sv.push_back (sound);
	}
}

//----------------------------------------------------------------------

void Audio::transferOwnershipOfSounds(Object const & previousOwner, Object const & newOwner, Plane const * const /*partition*/)
{
	for(const auto& it : s_soundIdToSoundMap)
	{
		Sound2 * const sound = it.second;
		
		if (nullptr == sound || sound->getObject() != &previousOwner)
			continue;

		// @todo - handle partition
		sound->setObject(&newOwner);
	}
}


//----------------------------------------------------------------------

Sound2 * Audio::getSoundById (const SoundId & soundId)
{
	const SoundIdToSoundMap::iterator it = s_soundIdToSoundMap.find(soundId);

	if (it != s_soundIdToSoundMap.end())
		return (*it).second;

	return nullptr;
}

//-----------------------------------------------------------------------------

void Audio::setDebugEnabled(bool const enabled)
{
	s_debugVisuals = enabled;
}

//-----------------------------------------------------------------------------

bool Audio::isDebugEnabled()
{
	return s_debugVisuals;
}

//-----------------------------------------------------------------------------

float Audio::getObstruction()
{
	return s_obstruction;
}

//-----------------------------------------------------------------------------

float Audio::getOcclusion()
{
	return s_occlusion;
}

//-----------------------------------------------------------------------------

void Audio::setSoundPosition_w(SoundId const &soundId, Vector const &position_w)
{
	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		iterSoundIdToSoundMap->second->setPosition_w(position_w);
	}
}

//-----------------------------------------------------------------------------

void Audio::setSoundVolume(SoundId const &soundId, float volume)
{
	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		iterSoundIdToSoundMap->second->setUserVolume(volume);
	}
}

//-----------------------------------------------------------------------------

void Audio::setSoundPitchDelta(SoundId const &soundId, float pitchDelta)
{
	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		iterSoundIdToSoundMap->second->setUserPitchDelta(pitchDelta);
	}
}

//-----------------------------------------------------------------------------

Vector Audio::getSoundPosition_w(SoundId const &soundId)
{
	Vector result;

	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		result = iterSoundIdToSoundMap->second->getPosition_w();
	}

	return result;
}

//-----------------------------------------------------------------------------

float Audio::getDistanceAtVolumeCutOff(SoundId const &soundId)
{
	float result = 0.0f;

	auto iterSoundIdToSoundMap = s_soundIdToSoundMap.find(soundId);

	if (iterSoundIdToSoundMap != s_soundIdToSoundMap.end())
	{
		result = iterSoundIdToSoundMap->second->getDistanceAtVolumeCutOff();
	}

	return result;
}

//-----------------------------------------------------------------------------

bool Audio::isToolApplication()
{
	return s_toolApplication;
}


//-----------------------------------------------------------------------------

void Audio::setToolApplication(bool const toolApplication)
{
	s_toolApplication = toolApplication;
}

//-----------------------------------------------------------------------------

void Audio::silenceAllNonBackgroundMusic()
{
	//DEBUG_REPORT_LOG(true, ("Audio: Silence\n"));

	++s_nonBackgroundFadeCount;
}

//-----------------------------------------------------------------------------

void Audio::unSilenceAllNonBackgroundMusic()
{
	//DEBUG_REPORT_LOG(true, ("Audio: UnSilence\n"));

	--s_nonBackgroundFadeCount;
}

//-----------------------------------------------------------------------------

void Audio::fadeAllNonVoiceover()
{
	++s_nonVoiceoverFadeCount;
}

//-----------------------------------------------------------------------------

void Audio::unFadeAllNonVoiceover()
{
	--s_nonVoiceoverFadeCount;
}

//-----------------------------------------------------------------------------

void Audio::fadeAll()
{
	++s_allAudioFadeCount;
}

//-----------------------------------------------------------------------------

void Audio::unfadeAll()
{
	--s_allAudioFadeCount;
	
	if(s_allAudioFadeCount < 0)
	{
		DEBUG_WARNING(true, ("Audio fade all count below 0"));
		s_allAudioFadeCount = 0;
	}
}

//-----------------------------------------------------------------------------

float Audio::getDefaultMasterVolume()
{
	return 1.0f;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultSoundEffectVolume()
{
	return 1.0f;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultAmbientEffectVolume()
{
	return 1.0f;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultBackGroundMusicVolume()
{
	return 0.75f;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultPlayerMusicVolume()
{
	return 0.5f;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultUserInterfaceVolume()
{
	return 0.75f;
}

//-----------------------------------------------------------------------------

void Audio::setFadeAllFactor(float factor)
{
	s_allAudioFadeFactor = factor;
}

//-----------------------------------------------------------------------------

float Audio::getFadeAllFactor()
{
	return s_allAudioFadeFactor;
}

//-----------------------------------------------------------------------------

float Audio::getDefaultFadeAllFactor()
{
	return 0.5f;
}

//-----------------------------------------------------------------------------

bool Audio::queueSample(Sound2 & sound, bool const soundIsAlreadyPlaying)
{
	bool result = true;
	DEBUG_FATAL(sound.getDistanceSquaredFromListener() > sqr(sound.getDistanceAtVolumeCutOff()), ("The sound is out of audible range, why is the sample trying to be queued?"));

	if (sound.getTemplate()->getAttenuationMethod() == AM_none)
	{
		if (!soundIsAlreadyPlaying)
		{
			s_centerBucket.emplace_back(&sound);
			//DEBUG_REPORT_LOG(true, ("Audio::queueSample() s_centerBucket.push_back1(%s)\n", sound.getTemplate()->getName()));
		}
		
		return result;
	}

	// Find the bucket to put the sound in
	Vector const listenerPosition_w(s_listenerObject != nullptr ? s_listenerObject->getPosition_w() : Vector::zero);
	Vector const position_w(sound.getPosition_w());
	float const distanceFromListenerSquared = listenerPosition_w.magnitudeBetweenSquared(position_w);

	if (distanceFromListenerSquared <= s_centerConsolidateDistanceSquared)
	{
		if (!soundIsAlreadyPlaying)
		{
			s_centerBucket.emplace_back(&sound);
			//DEBUG_REPORT_LOG(true, ("Audio::queueSample() s_centerBucket.push_back2(%s)\n", sound.getTemplate()->getName()));
		}
	}
	else if (s_listenerObject != nullptr)
	{
		// Rotate the sound to the object space of the listener
		Vector const position_l(s_listenerObject->rotateTranslate_w2o(position_w));

		float const absolutePositionX_l = fabsf(position_l.x);
		float const absolutePositionZ_l = fabsf(position_l.z);

		if (position_l.z > 0.0f)
		{
			if (position_l.x > 0.0f)
			{
				if (position_l.x <= position_l.z)
				{
					// Front
					result = AudioNamespace::queueSample(s_frontBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
				else
				{
					// Right
					result = AudioNamespace::queueSample(s_rightBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
			}
			else
			{
				if (absolutePositionX_l <= position_l.z)
				{
					// Front
					result = AudioNamespace::queueSample(s_frontBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
				else
				{
					// Left
					result = AudioNamespace::queueSample(s_leftBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
			}
		}
		else
		{
			if (position_l.x > 0.0f)
			{
				if (position_l.x <= absolutePositionZ_l)
				{
					// Back
					result = AudioNamespace::queueSample(s_backBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
				else
				{
					// Right
					result = AudioNamespace::queueSample(s_rightBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
			}
			else
			{
				if (absolutePositionX_l <= absolutePositionZ_l)
				{
					// Back
					result = AudioNamespace::queueSample(s_backBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
				else
				{
					// Left
					result = AudioNamespace::queueSample(s_leftBucket, sound, distanceFromListenerSquared, soundIsAlreadyPlaying);
				}
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void Audio::silenceNonBufferedMusic(bool silence)
{
	s_silenceNonBufferedMusic = silence;
}

// ============================================================================

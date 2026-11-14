/*
 * P710 native snapshot update: sgp/native_soundman.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_SOUNDMAN_H
#define SGP_NATIVE_SOUNDMAN_H

#include "core_types.h"
#include <cstddef>

/*
#ifdef __cplusplus
extern "C" {
#endif
*/

// Sample status flags
#define		SAMPLE_ALLOCATED		0x00000001
#define		SAMPLE_LOCKED			0x00000002
#define		SAMPLE_RANDOM			0x00000004
#define		SAMPLE_RANDOM_MANUAL	0x00000008

// Sound error values (they're all the same)
#define		NO_SAMPLE				0xffffffff
#define		SOUND_ERROR				0xffffffff

// Maximum allowable priority value
#define		PRIORITY_MAX			0xfffffffe
#define		PRIORITY_RANDOM			PRIORITY_MAX-1


// Structure definition for sound parameters being passed down to the sample playing function
typedef struct {
				UINT32			uiSpeed;
				UINT32			uiPitchBend;					// Random pitch bend range +/-
				UINT32			uiVolume;
				UINT32			uiPan;
				UINT32			uiLoop;
				UINT32			uiPriority;
				void			(*EOSCallback)(void *);
				void			*pCallbackData;
				} SOUNDPARMS;


// Structure definition for parameters to the random sample playing function
typedef struct {
				UINT32			uiTimeMin, uiTimeMax;
				UINT32			uiSpeedMin, uiSpeedMax;
				UINT32			uiVolMin, uiVolMax;
				UINT32			uiPanMin, uiPanMax;
				UINT32			uiPriority;
				UINT32			uiMaxInstances;
				} RANDOMPARMS;


// Global startup/shutdown functions
extern BOOLEAN	InitializeSoundManager(void);
extern void		ShutdownSoundManager(void);

// Configuration functions
extern BOOLEAN	SoundSetMemoryLimit(UINT32 uiLimit);
extern BOOLEAN	SoundSetCacheThreshhold(UINT32 uiThreshold);
extern void *	SoundGetDriverHandle( void );

// Master volume control functions
extern void		SoundSetDefaultVolume(UINT32 uiVolume);
extern UINT32	SoundGetDefaultVolume(void);

// Cache control functions
//extern UINT32	SoundLoadSample(STR pFilename);
UINT32	SoundLoadSample(STR pFilename);

extern UINT32	SoundFreeSample(STR pFilename);
extern UINT32	SoundLockSample(STR pFilename);
extern UINT32	SoundUnlockSample(STR pFilename);
extern BOOLEAN	SoundEmptyCache(void);

// Play/service sample functions
extern UINT32	SoundPlay(STR pFilename, SOUNDPARMS *pParms);
extern UINT32	SoundPlayStreamedFile( STR pFilename, SOUNDPARMS *pParms );
extern UINT32	SoundPlayFromBuffer(const char *name, const void *soundData, UINT32 size, SOUNDPARMS *params);
extern UINT32	SoundPlayRandom(STR pFilename, RANDOMPARMS *pParms);
extern BOOLEAN	SoundServiceStreams(void);
extern BOOLEAN	SoundServiceRandom(void);

// sevenfm
void ResetSoundMap(void);

// Sound instance manipulation functions
extern BOOLEAN	SoundStopAll(void);
extern BOOLEAN	SoundStopAllRandom(void);
extern BOOLEAN	SoundStop(UINT32 uiSoundID);
extern BOOLEAN	SoundIsPlaying(UINT32 uiSoundID);
extern BOOLEAN	SoundSetVolume(UINT32 uiSoundID, UINT32 uiVolume);
extern BOOLEAN	SoundSetPan(UINT32 uiSoundID, UINT32 uiPan);
extern UINT32	SoundGetVolume(UINT32 uiSoundID);
extern UINT32	SoundGetPosition(UINT32 uiSoundID);

extern void SoundSetSampleFlags( UINT32 uiSample, UINT32 uiFlags );
extern void SoundRemoveSampleFlags( UINT32 uiSample, UINT32 uiFlags );

extern void SoundEnableSound(BOOLEAN fEnable);

extern void SoundLog(CHAR8 *strMessage);

/*
#ifdef __cplusplus
}
#endif
*/

// Native contract: game-facing calls and EOS callbacks run on the calling/main
// thread. SDL's audio callback only renders PCM and marks completed voices.
// Service streams once per frame (MusicPoll already does this), including while
// music is disabled, so EOS callbacks and expired voice resources are reclaimed.
// Approved pinned FFmpeg decodes WAV/MP3/OGG exclusively through FileMan/VFS.
// Streamed files use a finite16384-frame PCM ring, prefetched/refilled here on
// the main thread. Call stream service each frame; a starved ring remains live
// until refilled rather than pretending EOF. No decode/I/O in SDL callbacks.
const char* NativeSoundLastError();
BOOLEAN NativeSoundPlaybackEnabled();
// Stream outcome remains observable after the actual EOS retirement. Terminal
// records are bounded to the latest256 stream voices; Unknown is explicit.
enum class NativeSoundStreamOutcome { Unknown, Playing, Finished, Stopped, Failed };
NativeSoundStreamOutcome NativeSoundGetStreamOutcome(UINT32 id);
// Game option bridge for gGameExternalOptions.fLimitSimultaneousSound. The
// engine owner must set this when options load/change (default false).
void NativeSoundSetSimultaneousLimit(BOOLEAN enabled);
// Bind the engine's original Random(range), including BMP_RANDOM/fNewRandom
// policy, when integrating gameplay. A deterministic local generator serves
// isolated native tests until the engine provides this callback.
void NativeSoundSetRandomSource(UINT32 (*source)(UINT32));
// Deterministic verification uses precisely the production PCM render path.
// Select manual mode before initialization: SDL still opens a real paused device.
// Never enable this for normal game playback. Stereo float PCM, 48000 Hz.
void NativeSoundSetManualRendering(BOOLEAN enabled);
BOOLEAN NativeSoundRender(float* stereo, UINT32 frames);
UINT32 NativeSoundActiveVoices();
UINT32 NativeSoundCachedBytes();
// Retained PCM capacities plus stream ring/adapter staging. Transient decoder
// work/reallocation peaks and opaque external-library allocations are excluded.
std::size_t NativeSoundRetainedBytes();
// Read-only bounded-stream evidence; includes caller/main-thread decode staging
// and AVIO/ring buffers, excludes opaque FFmpeg internal codec allocations.
UINT32 NativeSoundStreamBufferedFrames(UINT32 id);
std::uint64_t NativeSoundStreamSourceBytesRead(UINT32 id);
std::size_t NativeSoundStreamBufferedBytes(UINT32 id);
UINT32 NativeSoundStreamUnderruns(UINT32 id);
#endif

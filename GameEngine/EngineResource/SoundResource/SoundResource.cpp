#include <FMOD/fmod.hpp>
#include "SoundResource.h"
ENGINE_USE

SoundResource::~SoundResource() { if (m_pSound) m_pSound->release(); }


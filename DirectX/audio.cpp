
#include "main.h"
#include "audio.h"
#include <cmath>
#include <algorithm>





IXAudio2*				Audio::m_Xaudio = NULL;
IXAudio2MasteringVoice*	Audio::m_MasteringVoice = NULL;


void Audio::InitMaster()
{
	// XAudio生成
	ThrowIfFailed(XAudio2Create(&m_Xaudio, 0), "XAudio2Create");

	// マスタリングボイス生成
	ThrowIfFailed(m_Xaudio->CreateMasteringVoice(&m_MasteringVoice),
		"IXAudio2::CreateMasteringVoice");
}


void Audio::UninitMaster()
{
	if (m_MasteringVoice != nullptr)
	{
		m_MasteringVoice->DestroyVoice();
		m_MasteringVoice = nullptr;
	}
	SafeRelease(m_Xaudio);
}









void Audio::Load(const char *FileName)
{

	// サウンドデータ読込
	WAVEFORMATEX wfx = { 0 };

	{
		HMMIO hmmio = NULL;
		MMIOINFO mmioinfo = { 0 };
		MMCKINFO riffchunkinfo = { 0 };
		MMCKINFO datachunkinfo = { 0 };
		MMCKINFO mmckinfo = { 0 };
		UINT32 buflen;
		LONG readlen;


		hmmio = mmioOpen(const_cast<LPSTR>(FileName), &mmioinfo, MMIO_READ);
		if (hmmio == nullptr)
		{
			throw std::runtime_error(std::string("Audio file not found: ") + FileName);
		}

		riffchunkinfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		if (mmioDescend(hmmio, &riffchunkinfo, NULL, MMIO_FINDRIFF) != MMSYSERR_NOERROR)
		{
			mmioClose(hmmio, 0);
			throw std::runtime_error(std::string("Invalid WAVE RIFF chunk: ") + FileName);
		}

		mmckinfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		if (mmioDescend(hmmio, &mmckinfo, &riffchunkinfo, MMIO_FINDCHUNK) != MMSYSERR_NOERROR)
		{
			mmioClose(hmmio, 0);
			throw std::runtime_error(std::string("WAVE format chunk not found: ") + FileName);
		}

		if (mmckinfo.cksize >= sizeof(WAVEFORMATEX))
		{
			if (mmioRead(hmmio, reinterpret_cast<HPSTR>(&wfx), sizeof(wfx)) != sizeof(wfx))
			{
				mmioClose(hmmio, 0);
				throw std::runtime_error(std::string("Failed to read WAVE format: ") + FileName);
			}
		}
		else
		{
			PCMWAVEFORMAT pcmwf = { 0 };
			if (mmioRead(hmmio, reinterpret_cast<HPSTR>(&pcmwf), sizeof(pcmwf)) != sizeof(pcmwf))
			{
				mmioClose(hmmio, 0);
				throw std::runtime_error(std::string("Failed to read PCM format: ") + FileName);
			}
			memset(&wfx, 0x00, sizeof(wfx));
			memcpy(&wfx, &pcmwf, sizeof(pcmwf));
			wfx.cbSize = 0;
		}
		mmioAscend(hmmio, &mmckinfo, 0);

		datachunkinfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(hmmio, &datachunkinfo, &riffchunkinfo, MMIO_FINDCHUNK) != MMSYSERR_NOERROR ||
			datachunkinfo.cksize == 0 ||
			wfx.nBlockAlign == 0)
		{
			mmioClose(hmmio, 0);
			throw std::runtime_error(std::string("Invalid WAVE data chunk: ") + FileName);
		}



		buflen = datachunkinfo.cksize;
		m_SoundData = new unsigned char[buflen];
		readlen = mmioRead(hmmio, reinterpret_cast<HPSTR>(m_SoundData), buflen);
		if (readlen <= 0 || static_cast<UINT32>(readlen) != buflen)
		{
			mmioClose(hmmio, 0);
			delete[] m_SoundData;
			m_SoundData = nullptr;
			throw std::runtime_error(std::string("Failed to read WAVE samples: ") + FileName);
		}


		m_Length = readlen;
		m_PlayLength = readlen / wfx.nBlockAlign;


		mmioClose(hmmio, 0);
	}


	// サウンドソース生成
	ThrowIfFailed(m_Xaudio->CreateSourceVoice(&m_SourceVoice, &wfx),
		"IXAudio2::CreateSourceVoice");
}


void Audio::Uninit()
{
	if (m_SourceVoice != nullptr)
	{
		m_SourceVoice->Stop();
		m_SourceVoice->DestroyVoice();
		m_SourceVoice = nullptr;
	}

	delete[] m_SoundData;
	m_SoundData = nullptr;
	m_Length = 0;
	m_PlayLength = 0;
}

void Audio::LoadTone(float StartHz, float EndHz, float Seconds, float Volume)
{
	if (StartHz <= 0 || EndHz <= 0 || Seconds <= 0 || Seconds > 2 ||
		!std::isfinite(StartHz) || !std::isfinite(EndHz) || !std::isfinite(Seconds))
		throw std::runtime_error("Invalid sound synthesis parameters");
	Uninit();
	constexpr int sampleRate = 22050;
	m_PlayLength = static_cast<int>(Seconds * sampleRate);
	m_Length = m_PlayLength * sizeof(short);
	m_SoundData = new BYTE[m_Length];
	auto* samples = reinterpret_cast<short*>(m_SoundData);
	float phase = 0.0f;
	for (int i = 0; i < m_PlayLength; ++i)
	{
		const float t = static_cast<float>(i) / m_PlayLength;
		phase += XM_2PI * (StartHz + (EndHz - StartHz) * t) / sampleRate;
		const float envelope = std::min(1.0f, t * 30.0f) * (1.0f - t) * (1.0f - t);
		samples[i] = static_cast<short>(std::sin(phase) * envelope *
			std::clamp(Volume, 0.0f, 0.5f) * 32767.0f);
	}
	WAVEFORMATEX format{};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = sampleRate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = 2;
	format.nAvgBytesPerSec = sampleRate * 2;
	ThrowIfFailed(m_Xaudio->CreateSourceVoice(&m_SourceVoice, &format), "Create synthesized sound");
}





void Audio::Play(bool Loop)
{
	if (m_SourceVoice == nullptr || m_SoundData == nullptr)
	{
		throw std::runtime_error("Audio::Play called before Audio::Load");
	}

	ThrowIfFailed(m_SourceVoice->Stop(), "IXAudio2SourceVoice::Stop");
	ThrowIfFailed(m_SourceVoice->FlushSourceBuffers(), "IXAudio2SourceVoice::FlushSourceBuffers");


	// バッファ設定
	XAUDIO2_BUFFER bufinfo;

	memset(&bufinfo, 0x00, sizeof(bufinfo));
	bufinfo.AudioBytes = m_Length;
	bufinfo.pAudioData = m_SoundData;
	bufinfo.PlayBegin = 0;
	bufinfo.PlayLength = m_PlayLength;

	// ループ設定
	if (Loop)
	{
		bufinfo.LoopBegin = 0;
		bufinfo.LoopLength = m_PlayLength;
		bufinfo.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	ThrowIfFailed(m_SourceVoice->SubmitSourceBuffer(&bufinfo, NULL),
		"IXAudio2SourceVoice::SubmitSourceBuffer");

/*
	float outputMatrix[4] = { 0.0f , 0.0f, 1.0f , 0.0f };
	m_SourceVoice->SetOutputMatrix(m_MasteringVoice, 2, 2, outputMatrix);
	//m_SourceVoice->SetVolume(0.1f);
*/


	// 再生
	ThrowIfFailed(m_SourceVoice->Start(), "IXAudio2SourceVoice::Start");

}




#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <string>

#pragma comment(lib, "winmm.lib")

// ============================================================
// SoundManager
// WinAPI 프로젝트용 간단 사운드 매니저
//
// BGM : mciSendStringW 사용. wav/mp3 루프 재생용.
// SFX : PlaySoundW 사용. wav 단발 효과음용.
//
// 상대 경로는 아래 순서로 자동 탐색한다.
// 1) Resource\Sound\xxx.wav
// 2) Tower_Triumph\Resource\Sound\xxx.wav
// 3) ..\Tower_Triumph\Resource\Sound\xxx.wav
// ============================================================

class SoundManager
{
public:
	static bool Exists(const std::wstring& filePath)
	{
		DWORD attr = GetFileAttributesW(filePath.c_str());

		return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	static std::wstring ResolvePath(const std::wstring& filePath)
	{
		if (Exists(filePath))
		{
			return filePath;
		}

		std::wstring candidate = L"Tower_Triumph\\" + filePath;
		if (Exists(candidate))
		{
			return candidate;
		}

		candidate = L"..\\Tower_Triumph\\" + filePath;
		if (Exists(candidate))
		{
			return candidate;
		}

		return filePath;
	}

	static bool PlaySFX(const std::wstring& filePath)
	{
		std::wstring resolvedPath = ResolvePath(filePath);

		if (!Exists(resolvedPath))
		{
			return false;
		}

		BOOL result = PlaySoundW(
			resolvedPath.c_str(),
			NULL,
			SND_FILENAME | SND_ASYNC | SND_NODEFAULT
		);

		return result == TRUE;
	}

	static void StopSFX()
	{
		PlaySoundW(NULL, NULL, 0);
	}

	static bool PlayBGM(const std::wstring& filePath, bool loop = true)
	{
		StopBGM();

		std::wstring resolvedPath = ResolvePath(filePath);

		if (!Exists(resolvedPath))
		{
			return false;
		}

		// 확장자 자동 판정으로 먼저 열고, 실패하면 waveaudio로 한 번 더 시도한다.
		std::wstring openCmd = L"open \"" + resolvedPath + L"\" alias TT_BGM";
		MCIERROR openResult = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);

		if (openResult != 0)
		{
			openCmd = L"open \"" + resolvedPath + L"\" type waveaudio alias TT_BGM";
			openResult = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
		}

		if (openResult != 0)
		{
			return false;
		}

		std::wstring playCmd = loop ? L"play TT_BGM repeat" : L"play TT_BGM";
		MCIERROR playResult = mciSendStringW(playCmd.c_str(), NULL, 0, NULL);

		return playResult == 0;
	}

	static void StopBGM()
	{
		mciSendStringW(L"stop TT_BGM", NULL, 0, NULL);
		mciSendStringW(L"close TT_BGM", NULL, 0, NULL);
	}
};

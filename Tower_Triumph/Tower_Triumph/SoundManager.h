#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <string>

#pragma comment(lib, "winmm.lib")


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
		if (Exists(filePath)) return filePath;

		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		std::wstring exeDir = exePath;
		size_t lastSlash = exeDir.find_last_of(L"\\/");
		if (lastSlash != std::wstring::npos) exeDir = exeDir.substr(0, lastSlash);

		std::wstring candidates[] = {
			exeDir + L"\\" + filePath,							
			exeDir + L"\\..\\" + filePath,						
			exeDir + L"\\..\\Tower_Triumph\\" + filePath,		
			exeDir + L"\\..\\..\\Tower_Triumph\\" + filePath,	
			L"Tower_Triumph\\" + filePath,						
			L"Tower_Triumph\\Tower_Triumph\\" + filePath		
		};

		for (const auto& cand : candidates)
		{
			if (Exists(cand))
			{
				OutputDebugStringW((L"[SoundManager] Found: " + cand + L"\n").c_str());
				return cand;
			}
		}

		OutputDebugStringW((L"[SoundManager] Failed to find: " + filePath + L"\n").c_str());
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

		wchar_t shortPath[MAX_PATH];
		if (GetShortPathNameW(resolvedPath.c_str(), shortPath, MAX_PATH) == 0)
		{
			wcsncpy_s(shortPath, resolvedPath.c_str(), _TRUNCATE);
		}

		std::wstring openCmd = L"open \"" + std::wstring(shortPath) + L"\" type mpegvideo alias TT_BGM";
		MCIERROR openResult = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);

		if (openResult != 0)
		{
			openCmd = L"open \"" + std::wstring(shortPath) + L"\" alias TT_BGM";
			openResult = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
		}

		if (openResult != 0)
		{
			wchar_t errText[MAX_PATH];
			mciGetErrorStringW(openResult, errText, MAX_PATH);
			OutputDebugStringW((L"[SoundManager] BGM Open Error (" + std::to_wstring(openResult) + L"): " + errText + L"\n").c_str());
			return false;
		}

		mciSendStringW(L"setaudio TT_BGM volume to 1000", NULL, 0, NULL);

		std::wstring playCmd = loop ? L"play TT_BGM repeat from 0" : L"play TT_BGM from 0";
		MCIERROR playResult = mciSendStringW(playCmd.c_str(), NULL, 0, NULL);

		if (playResult != 0)
		{
			OutputDebugStringW(L"[SoundManager] BGM Play Error\n");
		}

		return playResult == 0;
	}

	static void StopBGM()
	{
		mciSendStringW(L"stop TT_BGM", NULL, 0, NULL);
		mciSendStringW(L"close TT_BGM", NULL, 0, NULL);
	}
};

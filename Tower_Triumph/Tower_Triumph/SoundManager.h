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
		// 1. 이미 절대 경로이거나 현재 작업 디렉토리 기준 존재하면 즉시 반환
		if (Exists(filePath)) return filePath;

		// 2. 실행 파일(.exe) 위치 기준 탐색
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		std::wstring exeDir = exePath;
		size_t lastSlash = exeDir.find_last_of(L"\\/");
		if (lastSlash != std::wstring::npos) exeDir = exeDir.substr(0, lastSlash);

		// 탐색 후보지 리스트 (실행 파일 기준 상위 폴더들)
		std::wstring candidates[] = {
			exeDir + L"\\" + filePath,							// .exe와 같은 층
			exeDir + L"\\..\\" + filePath,						// .exe 한 단계 위 (Debug/Release 폴더 대응)
			exeDir + L"\\..\\Tower_Triumph\\" + filePath,		// .sln 위치 기준
			exeDir + L"\\..\\..\\Tower_Triumph\\" + filePath,	// 더 상위 기준
			L"Tower_Triumph\\" + filePath,						// 프로젝트 상대 경로
			L"Tower_Triumph\\Tower_Triumph\\" + filePath		// 중첩 프로젝트 상대 경로
		};

		for (const auto& cand : candidates)
		{
			if (Exists(cand))
			{
				// 성공 시 디버그 출력 (Visual Studio '출력' 창에서 확인 가능)
				OutputDebugStringW((L"[SoundManager] Found: " + cand + L"\n").c_str());
				return cand;
			}
		}

		// 실패 시 디버그 출력
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

		// [중요] MCI는 긴 경로(특히 공백 포함)에 취약함.
		// GetShortPathNameW를 사용하여 '8.3' 형식의 짧은 경로로 변환하여 전달.
		wchar_t shortPath[MAX_PATH];
		if (GetShortPathNameW(resolvedPath.c_str(), shortPath, MAX_PATH) == 0)
		{
			wcsncpy_s(shortPath, resolvedPath.c_str(), _TRUNCATE);
		}

		// 'mpegvideo' 타입이 현대 윈도우에서 가장 호환성이 좋음 (큰 wav/mp3 모두 대응)
		std::wstring openCmd = L"open \"" + std::wstring(shortPath) + L"\" type mpegvideo alias TT_BGM";
		MCIERROR openResult = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);

		if (openResult != 0)
		{
			// 실패 시 기본 타입으로 재시도
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

		// 볼륨을 최대로 설정 (0~1000)
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

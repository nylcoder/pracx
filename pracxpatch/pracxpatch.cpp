/*
 * pracxpatch.cpp
 *
 * Small script that opens terran and terranx.exe as files and writes some
 * assembly code into them that compels them to call the PRACX libraries
 * (prac.dll and prax.dll).
 *
 */

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

bool GetFileExists(char* pszFileName)
{
	HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return true;
	}
	return false;
}


#define _SMACLoadLibraryA		0068B0F4h
#define _SMACGetProcAddress	0068B0F0h

#define _SMAXLoadLibraryA		00669120h
#define _SMAXGetProcAddress	00669124h


__declspec(naked) void PRACXHookModelSMAC(void)
{
	_asm{
		call	jmp_over
			_emit 'p'
			_emit 'r'
			_emit 'a'
			_emit 'c'
			_emit '\0'
		jmp_over:
		call	ds : _SMACLoadLibraryA
			test	eax, eax
			jnz		loaded
			ret
		loaded :
		push    eax
			push	1
			push    eax
			call    ds : _SMACGetProcAddress
			test    eax, eax
			jnz     callit
			pop     eax
			ret
		callit :
		call    eax
			ret
			_emit  'd'
			_emit  'o'
			_emit  'n'
			_emit  'e'
			_emit  '\0'
	}
}

__declspec(naked) void PRACXHookModelSMAX(void)
{
	_asm{
		call	jmp_over
			_emit 'p'
			_emit 'r'
			_emit 'a'
			_emit 'x'
			_emit '\0'
		jmp_over:
		call	ds : _SMAXLoadLibraryA
			test	eax, eax
			jnz		loaded
			ret
		loaded :
		push    eax
			push	1
			push    eax
			call    ds : _SMAXGetProcAddress
			test    eax, eax
			jnz     callit
			pop     eax
			ret
		callit :
		call    eax
			ret
			_emit  'd'
			_emit  'o'
			_emit  'n'
			_emit  'e'
			_emit  '\0'
	}
}

int PRACXHookFile(char* pszFileName, int iType)
{
#define SMAC_HOOKADDRESS     0x006A6000
#define SMAC_LOADADDRESS     0x0068A160
#define SMAC_IMAGEBASE	     0x00400000

#define SMAX_HOOKADDRESS     0x00682000
#define SMAX_LOADADDRESS     0x00668160
#define SMAX_IMAGEBASE		 0x00400000


	ULONG AC_LOADADDRESS;
	ULONG AC_IMAGEBASE;
	ULONG AC_HOOKADDRESS;

	HANDLE hOut;

	if (iType == 0)
	{
		AC_LOADADDRESS = SMAC_LOADADDRESS;
		AC_IMAGEBASE = SMAC_IMAGEBASE;
		AC_HOOKADDRESS = SMAC_HOOKADDRESS;
	}
	else
	{
		AC_LOADADDRESS = SMAX_LOADADDRESS;
		AC_IMAGEBASE = SMAX_IMAGEBASE;
		AC_HOOKADDRESS = SMAX_HOOKADDRESS;
	}

	hOut = CreateFile(pszFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hOut == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	int iSize = GetFileSize(hOut, NULL);
	char* pBuffer = new char[iSize];
	ReadFile(hOut, pBuffer, iSize, (LPDWORD)&iSize, NULL);

	CloseHandle(hOut);

	char* pDest = &pBuffer[AC_LOADADDRESS - AC_IMAGEBASE];
	char* pSource = (char*)((iType == 0) ? PRACXHookModelSMAC : PRACXHookModelSMAX);

	int iLen;

	for (iLen = 0; 0 != strcmp(&pSource[iLen], "done"); iLen++);

	if (0 == memcmp(pSource, pDest, iLen))
	{
		delete[] pBuffer;
		return 1;
	}

	memcpy(pDest, pSource, iLen);

	*((UINT*)&pBuffer[AC_HOOKADDRESS - AC_IMAGEBASE]) = AC_LOADADDRESS;

	char szTmpFileName[1024];

	strcpy(szTmpFileName, pszFileName);
	strcat(szTmpFileName, ".tmp");

	hOut = CreateFile(szTmpFileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hOut == INVALID_HANDLE_VALUE)
	{
		delete[] pBuffer;
		return 0;
	}

	WriteFile(hOut, pBuffer, iSize, (LPDWORD)&iSize, NULL);

	CloseHandle(hOut);

	delete[] pBuffer;

	char szBackupFileName[1024];
	strcpy(szBackupFileName, pszFileName);
	strcat(szBackupFileName, ".orig");

	CopyFile(pszFileName, szBackupFileName, false);

	if (!CopyFile(szTmpFileName, pszFileName, false))
		return 0;

	DeleteFile(szTmpFileName);
	return 1;
}



int main(int argc, char* argv[])
{
	if (!GetFileExists("terran.exe"))
	{
		MessageBox(NULL, "No Alpha Centauri found to patch.", "Not Found", MB_OK);
	}
	else
	{
		int r = PRACXHookFile("terran.exe", 0);

		if (!r)
			MessageBox(NULL, "Error patching Alpha Centauri.", "Error", MB_OK);
		else
			MessageBox(NULL, "Alpha Centauri patched successfully.", "Success", MB_OK);

	}

	if (!GetFileExists("terranx.exe"))
	{
		MessageBox(NULL, "No Alien Crossfile found to patch.", "Not Found", MB_OK);
	}
	else
	{
		int r = PRACXHookFile("terranx.exe", 1);

		if (!r)
			MessageBox(NULL, "Error patching Alien Crossfile.", "Error", MB_OK);
		else
			MessageBox(NULL, "Alien Crossfile patched successfully.", "Success", MB_OK);

	}
		
	return 0;
}


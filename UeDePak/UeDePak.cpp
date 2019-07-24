// UeDePak.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "pch.h"
#include "UeDePak.h"
#include "detours.h"
#include "Console.h"
#include "Selector.h"
#include "Util.h"
#pragma comment(lib, "detours.lib")

extern ULONG_PTR GetModuleLen(HMODULE hModule);
extern ULONG_PTR FindPattern(ULONG_PTR dwdwAdd, ULONG_PTR dwLen, const BYTE *bMask, const char * szMask);
extern void InitConfig();
extern bool GetConfig(const std::string&& ver, const Config*& cfg);

EXTERN_C_START
PVOID JmpFunc = NULL;
ULONG_PTR OrgDencryptCode = NULL;
EXTERN_C_END

// ����ж�ع���
PVOID FixFunc = NULL;

VOID passFunc(unsigned char* bkey) {
  static std::string key;
  char binstr[3];
  
  if (!key.empty()) {
    return;
  }

#define GETBYTE(x) (*(BYTE*)(x))
  for (auto i = 0; i < 32; i++) {
    sprintf_s(binstr, "%02X", GETBYTE(bkey + i));
    key += binstr;
  }
#undef GETBYTE

  Write2Con(std::string("[+] -------- key is : 0x") + key + "\r\n");
}

bool setHook() {

  // ������Ը���Զ���ȡ�ļ��汾��Ϣ�Ķ���
  const Config* cfg;
  if (!GetConfig("4.21", cfg)) {
    Write2Con("[-] -------- Unsupported versions!\r\n");
    return false;
  }

  // ������ʶ���Ӳ��� & �����һ������
  JmpFunc = &passFunc;
  FixFunc = cfg->fix_jump;

  auto moduleBase = (ULONG_PTR)GetModuleHandle(0);

  // ���sig��һ�ֺ�Σ�յĶ�λ��ʽ,����ͬһ���汾�ڲ�ͬ���������¿��ܻ�õ���ͬ��sig
  // Ҳ����Ի����������õķ�ʽ
  // ���縴��������λ����ȡ���������������Եȷ�ʽ���������
  OrgDencryptCode = FindPattern(moduleBase, GetModuleLen((HMODULE)moduleBase), (const BYTE*)cfg->pass_sig, cfg->pass_mask);
  
  if (!OrgDencryptCode) {
    Write2Con("[-] -------- Get OrgDencryptCode fail!\r\n");
    return false;
  }
  else {
    char temp[20];
    sprintf_s(temp, "0x%016llX", OrgDencryptCode);
    Write2Con(std::string("[+] -------- OrgDencryptCode Pointer : ") + temp + "\r\n");
  }

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());

  if (OrgDencryptCode) DetourAttach((PVOID*)&OrgDencryptCode, FixFunc);

  return DetourTransactionCommit() == NO_ERROR;
}


bool dropHook() {
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());

  if (OrgDencryptCode) DetourDetach((PVOID*)&OrgDencryptCode, FixFunc);

  return DetourTransactionCommit() == NO_ERROR;
}




DWORD WINAPI Do(LPVOID lpThreadParameter) {

  // ԭ���Ǵ���������Ӹ������ļ���
  EnableConsole();
  InitConfig();

  Write2Con("[+] ---- Depak initialization complete----\r\n");
  Write2Con("[+] -------- Start checking signatures\r\n");
  
  if (setHook()) {
    Write2Con("[+] -------- Wait to get AES password\r\n");
  }
  else {
    Write2Con("[+] -------- Failed to get AES password\r\n");
  }

  return 0;
}

void Undo() {
  dropHook();
}
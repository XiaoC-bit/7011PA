#pragma once
/*
异常崩溃捕获 by.Lei
*/
#if defined(BREAKPAD) || defined(__APPLE__) || defined(__linux__)
#include "ToolDefine.h"
#include <mutex>
#include <fstream>

#ifdef _WIN32
#include <client/windows/handler/exception_handler.h>

#pragma comment(lib,"common.lib")
#pragma comment(lib,"crash_generation_client.lib")
#pragma comment(lib,"crash_generation_server.lib")
#pragma comment(lib,"exception_handler.lib")

#elif __APPLE__
#include <client/mac/handler/exception_handler.h>
#elif __linux__
#include <client/linux/handler/exception_handler.h>
#endif // _WIN32

bool dumpCallBack(const wchar_t* dump_path, const wchar_t* id,
	void* context, EXCEPTION_POINTERS* exinfo,
	MDRawAssertionInfo* assertion,
	bool succeeded)
{
	if (succeeded) {
		writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D")), qstringArg("时间:%1 创建转储文件[%2]", getCurTime("%Y-%M-%D %h:%m:%s:%z"), wstringToString(id)));
	}
	else {
		writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D")), qstringArg("时间:%1 创建转储文件失败", getCurTime("%Y-%M-%D %h:%m:%s:%z")));
	}
	return succeeded;
}

//设置崩溃生产异常文件
void setExceptionHandlers()
{
	google_breakpad::ExceptionHandler eh(stringToWstring(qstringArg("%1log", getAbsolutePath())), nullptr, dumpCallBack, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
}
#elif _WIN32

#include <windows.h>
#include <DbgHelp.h>
#include <signal.h>
#include <mutex>
#include <new>
#include <fstream>
#include "ToolDefine.h"
#pragma comment(lib,"Dbghelp.lib")

//创建dump文件
void exceptionHandler(EXCEPTION_POINTERS* excpInfo)
{
	//创建 Dump 文件
	std::string fileName = (qstringArg("%1log/%2.dmp", getAbsolutePath(), getCurTime("%Y%M%D%h%m%s")));
	
	HANDLE hDumpFile = CreateFile(stringToWstring(fileName).c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {
		//Dump信息
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = excpInfo;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;
		//写入Dump文件内容
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, excpInfo ? &dumpInfo : NULL, NULL, NULL);
		CloseHandle(hDumpFile);
	}
}

//取消异常过滤 加载DLL替换内部的函数
bool PreventSetUnhandledExceptionFilter()
{
#ifdef _M_IX86 //目前只找到x86版本 此处作用是防止被其他地方替换掉回调函数
	HMODULE hKernelDll = LoadLibrary(L"kernel32.dll");
	if (hKernelDll == NULL)
		return false;
	void* kernelFunction = (void*)GetProcAddress(hKernelDll, "SetUnhandledExceptionFilter");
	if (!kernelFunction) {
		return false;
	}
	unsigned char code[16];
	int size = 0;
	code[size++] = 0x33;
	code[size++] = 0xC0;
	code[size++] = 0xC2;
	code[size++] = 0x04;
	code[size++] = 0x00;
	DWORD dwOldFlag, dwTempFlag;
	VirtualProtect(kernelFunction, size, PAGE_READWRITE, &dwOldFlag);
	WriteProcessMemory(GetCurrentProcess(), kernelFunction, code, size, NULL);
	VirtualProtect(kernelFunction, size, dwOldFlag, &dwTempFlag);
#endif // _M_IX86

	return true;
}

//处理异常
void handlingExceptions(EXCEPTION_POINTERS* excpInfo)
{
	if (!excpInfo) {
		__try {
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
		}
		__except (exceptionHandler(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
		}
	}
	else {
		exceptionHandler(excpInfo);
	}
}

//崩溃异常处理
LONG WINAPI unhandledException(EXCEPTION_POINTERS* excpInfo = NULL)
{
	handlingExceptions(excpInfo);

	return 0;
}

//无效参数异常回调
void invalidParameter(const wchar_t* expr, const wchar_t* func,
	const wchar_t* file, unsigned int line, uintptr_t reserved)
{
	writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D%h%m%s")), qstringArg("[%1][%2][%3][%4] 异常停止", wstringToString(expr), wstringToString(func), wstringToString(file), line));
	unhandledException();
}

//虚函数异常回调
void pureVirtualCall()
{
	writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D%h%m%s")), "virtual 异常停止");
	unhandledException();
}

//新建内存失败回调
void newMemoryFail()
{
	writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D%h%m%s")), "new 异常停止");
	unhandledException();
}

//abort异常回调
void sigAbortHandler(int sig)
{
	writeFile(qstringArg("%1log/%2_exception.txt", getAbsolutePath(), getCurTime("%Y%M%D%h%m%s")), qstringArg("abort[%1] 异常停止", sig));
	signal(SIGABRT, sigAbortHandler);
	unhandledException();
}

//设置崩溃生产异常文件
void setExceptionHandlers()
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);  //设置错误模式不显示弹窗
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, 0));
	SetUnhandledExceptionFilter(unhandledException);  //设置异常崩溃回调
	_set_invalid_parameter_handler(invalidParameter); //置处理无效的参数异常回调
	_set_purecall_handler(pureVirtualCall);  //设置处理虚函数异常回调
	signal(SIGABRT, sigAbortHandler);  //设置处理abort异常回调
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	std::set_new_handler(newMemoryFail);  //新建内存失败回调
}

#endif

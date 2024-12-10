#include "ToolDefine.h"
#ifdef _WIN32
#include <comutil.h>  
#include <io.h>
#include <qregexp.h>
#include <qstring.h>
#include <qlist.h>
#include <direct.h>
#pragma comment(lib, "comsuppw.lib")
#elif __APPLE__ || __linux__
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#endif // _WIN32
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <chrono>
#include <regex>
#include <fstream>

//字符到int
int fromHex(char ch)
{
	return ((ch >= '0') && (ch <= '9')) ? int(ch - '0') :
		((ch >= 'A') && (ch <= 'F')) ? int(ch - 'A' + 10) :
		((ch >= 'a') && (ch <= 'f')) ? int(ch - 'a' + 10) : -1;
}



//HEX转字符串 0不转换 1转换为小写 2转换为大写
std::string hexToString(const std::string& str, int type, const std::string& prefix)
{
	std::stringstream out;
	out << std::hex;
	for (auto it = str.begin(); it != str.end(); it++) {
		out << prefix;
		out << std::setw(2) << std::setfill('0');
		out << (static_cast<short>(*it) & 0xff);
	}
	std::string data = out.str();
	if (type == 1)
		std::transform(data.begin(), data.end(), data.begin(), tolower);
	else if (type == 2)
		std::transform(data.begin(), data.end(), data.begin(), toupper);
	return data;
}

//获取目录文件
void getPathFiles(std::string path, std::list<std::string>& files)
{
#ifdef _WIN32
	//文件句柄  
	intptr_t lFile = 0;
	//文件信息  
	struct _finddata_t fileinfo;
	std::string p;
	if ((lFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
		do {
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib & _A_SUBDIR)) {
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getPathFiles(p.assign(path).append("/").append(fileinfo.name), files);
			}
			else {
				files.push_back(p.assign(path).append("/").append(fileinfo.name));
			}
		} while (_findnext(lFile, &fileinfo) == 0);
		_findclose(lFile);
	}

#elif __APPLE__ || __linux__
	DIR* dir;
	if (!(dir = opendir(path.c_str()))) {
		return;
	}
	struct dirent* dp = NULL; /* readdir函数的返回值就存放在这个结构体中 */
	struct stat st;
	std::string curPath;
	while ((dp = readdir(dir)) != NULL) {
		/* 把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录 */
		if ((!strncmp(dp->d_name, ".", 1)) || (!strncmp(dp->d_name, "..", 2)))
			continue;
		curPath = qstringArg("%1/%2", path, dp->d_name);
		stat(curPath.c_str(), &st);
		if (!S_ISDIR(st.st_mode)) {
			files.push_back(curPath);
		}
		else {
			if (!(dir = opendir(path.c_str()))) {
				return;
			}
		}
	}
	closedir(dir);
#endif // _WIN32
}

//正则匹配目录中文件
void getFiles(QString  path, QList<QString>& files, QRegExp re)
{

	//文件句柄  
	intptr_t lFile = 0;
	//文件信息  
	struct _finddata_t fileinfo;
	std::string p;
	if ((lFile = _findfirst(p.assign(path.toStdString()).append("\\*").c_str(), &fileinfo)) != -1) {
		do {
			//判断是否匹配正则，匹配则加入列表  
			if (re.exactMatch(fileinfo.name)) {
				files.append(fileinfo.name);
			}
		} while (_findnext(lFile, &fileinfo) == 0);
		_findclose(lFile);
	}
}

//字符替换
std::string stringReplace(const std::string& str, const std::string& before, const std::string& after, int replaceNum)
{
	std::string data = str;
	stringReplace(data, before, after, replaceNum);
	return data;
}

//字符串查询替换
void stringReplace(std::string& str, const std::string& before, const std::string& after, int replaceNum)
{
	int curReplaceNum = 0;
	for (std::string::size_type pos(0); pos != std::string::npos; pos += after.length()) {
		pos = str.find(before, pos);
		if (pos != std::string::npos) {
			str.replace(pos, before.length(), after);
			if (replaceNum != 0 && ++curReplaceNum >= replaceNum) {
				break;
			}
		}
		else
			break;
	}
}

//字符串分割
std::vector<std::string> stringSplit(const std::string& str, const std::string& delim)
{
	std::vector<std::string> elems;
	size_t ullPos = 0;
	size_t ullLen = str.length();
	size_t ullDelimLen = delim.length();
	if (ullDelimLen == 0)
		return elems;
	while (ullPos < ullLen) {
		size_t ullFindPos = str.find(delim, ullPos);
		if (ullFindPos == std::string::npos) {
			elems.push_back(str.substr(ullPos, ullLen - ullPos));
			break;
		}
		elems.push_back(str.substr(ullPos, ullFindPos - ullPos));
		ullPos = ullFindPos + ullDelimLen;
	}

	return elems;
}

//字符串分割 正则
std::vector<std::string> stringSplitRegex(const std::string& str, const std::string& regex,
	std::regex_constants::match_flag_type flgs)
{
	std::vector<std::string> list;
	try {
		std::regex re(regex);
		std::smatch sm;
		size_t curPos = 0;
		while (std::regex_search(str.begin() + curPos, str.end(), sm, re, flgs)) {
			size_t pos = sm.position();
			if (!sm.empty()) {
				list.push_back(str.substr(curPos, pos));
				pos += sm[0].str().size();
			}
			curPos += pos;
		}

		if (curPos < str.size()) {
			list.push_back(str.substr(curPos, str.size() - curPos));
		}
	}
	catch (const std::regex_error& /*err*/) {
		//std::cout << err.what() << std::endl;
	}


	return list;
}

//获取程序运行路径
void getWorkPath(std::string& path)
{
	path = getWorkPath();
}

//获取程序工作路径
std::string getWorkPath()
{
	std::string path;
	char szBuf[1024];
	zeroMemory(szBuf, sizeof(szBuf));
	if (getcwd(szBuf, sizeof(szBuf)) == nullptr) {
		return path;
	}
	path = szBuf;
	return path;
}

//获取程序文件绝对路径
void getAbsolutePath(std::string& path)
{
	path = getAbsolutePath();
}

std::string TCHARToUTF8(const std::basic_string<TCHAR>& wstr)
{
	// 对于Unicode版本，wstr到utf8的转换
	if (sizeof(TCHAR) == sizeof(char)) {
		return std::string(wstr.begin(), wstr.end());
	}

	std::string utf8Str;
	int wstrLength = (int)wstr.size();
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstrLength, NULL, 0, NULL, NULL);

	if (bufferSize > 0) {
		std::vector<char> buffer(bufferSize);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstrLength, buffer.data(), bufferSize, NULL, NULL);
		utf8Str = std::string(buffer.begin(), buffer.end() - 1); // 移除终止字符
	}

	return utf8Str;
}

//获取程序文件绝对路径
std::string getAbsolutePath()
{
	std::string exePath;
#ifdef _WIN32
	TCHAR szPath[MAX_PATH] = { 0 };
	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
		return exePath;
	}
	//exePath = std::string(szPath);
	exePath = TCHARToUTF8(szPath);
	exePath = stringReplaceRegex(exePath, "[^/|\\\\]+?exe", "");
#elif __linux__
	char szPath[1024];
	zeroMemory(szPath, sizeof(szPath));
	readlink("/proc/self/exe", szPath, sizeof(szPath));
	exePath = szPath;
	exePath = stringReplaceRegex(exePath, "[^/]+$", "");
#elif __APPLE__
	char szPath[1024];
	zeroMemory(szPath, sizeof(szPath));
	uint32_t size = 1024;
	_NSGetExecutablePath(szPath, &size);
	exePath = szPath;
	exePath = stringReplaceRegex(exePath, "[^/]+$", "");
#endif // _WIN32

	return exePath;
}


//string转 wstring
std::wstring stringToWstring(const std::string& data)
{
	//效率不行，高并发时容易失败弃用
	/*setlocale(LC_ALL, "chs");
	const char* source = data.c_str();
	size_t size = data.size() + 1;
	wchar_t* dest = new wchar_t[size];
	wmemset(dest, 0, size);
	mbstowcs(dest, source, size);

	std::wstring result = dest;
	delete[] dest;
	setlocale(LC_ALL, "C");
	return result;*/
	_bstr_t t = data.c_str();
	wchar_t* pwchar = (wchar_t*)t;
	std::wstring result = pwchar;
	return result;
}

//wstring转string
std::string wstringToString(const std::wstring& data)
{
	//效率不行，高并发时容易失败弃用
	/*std::string curLocale = setlocale(LC_ALL, NULL);
	setlocale(LC_ALL, "chs");
	const wchar_t* source = data.c_str();
	size_t size = 2 * data.size() + 1;
	char* dest = new char[size];
	memset(dest, 0, size);
	wcstombs(dest, source, size);

	std::string result = dest;
	delete[] dest;

	setlocale(LC_ALL, curLocale.c_str());
	return result;*/
	_bstr_t t = data.c_str();
	char* pchar = (char*)t;
	std::string result = pchar;
	return result;
}

//获取时间戳 毫秒
long long getTimeStamp()
{
	auto timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	return timeNow.count();
}

//获取当前时间
//%Y-%M-%D %h:%m:%s:%z
std::string getCurTime(const std::string& format)
{
#ifdef _WIN32
	/*SYSTEMTIME t;
	GetLocalTime(&t);
	std::string strTime = format;
	stringReplace(strTime, "%Y", numToString(t.wYear));
	stringReplace(strTime, "%M", qArg(t.wMonth, 2));
	stringReplace(strTime, "%D", qArg(t.wDay, 2));
	stringReplace(strTime, "%h", qArg(t.wHour, 2));
	stringReplace(strTime, "%m", qArg(t.wMinute, 2));
	stringReplace(strTime, "%s", qArg(t.wSecond, 2));
	stringReplace(strTime, "%z", qArg(t.wMilliseconds, 3));*/
	time_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* tmInfo = localtime(&timeNow);
	std::string strTime = format;
	stringReplace(strTime, "%Y", numToString(tmInfo->tm_year + 1900));
	stringReplace(strTime, "%M", qArg(tmInfo->tm_mon + 1, 2));
	stringReplace(strTime, "%D", qArg(tmInfo->tm_mday, 2));
	stringReplace(strTime, "%h", qArg(tmInfo->tm_hour, 2));
	stringReplace(strTime, "%m", qArg(tmInfo->tm_min, 2));
	stringReplace(strTime, "%s", qArg(tmInfo->tm_sec, 2));
	stringReplace(strTime, "%z", qArg(numToString(getTimeStamp()).substr(10, 3), 3));

	return strTime;
#elif __APPLE__ || __linux__
	/*struct timeval tv;
	struct timezone tz;
	struct tm* tmInfo;
	gettimeofday(&tv, &tz);
	tmInfo = localtime(&tv.tv_sec);
	std::string strTime = format;
	stringReplace(strTime, "%Y", numToString(tmInfo->tm_year + 1900));
	stringReplace(strTime, "%M", qArg(tmInfo->tm_mon, 2));
	stringReplace(strTime, "%D", qArg(tmInfo->tm_mday, 2));
	stringReplace(strTime, "%h", qArg(tmInfo->tm_hour, 2));
	stringReplace(strTime, "%m", qArg(tmInfo->tm_min, 2));
	stringReplace(strTime, "%s", qArg(tmInfo->tm_sec, 2));
	stringReplace(strTime, "%z", qArg(tv.tv_usec / 1000, 3));

	return strTime;*/
	time_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* tmInfo = localtime(&timeNow);
	std::string strTime = format;
	stringReplace(strTime, "%Y", numToString(tmInfo->tm_year + 1900));
	stringReplace(strTime, "%M", qArg(tmInfo->tm_mon + 1, 2));
	stringReplace(strTime, "%D", qArg(tmInfo->tm_mday, 2));
	stringReplace(strTime, "%h", qArg(tmInfo->tm_hour, 2));
	stringReplace(strTime, "%m", qArg(tmInfo->tm_min, 2));
	stringReplace(strTime, "%s", qArg(tmInfo->tm_sec, 2));
	stringReplace(strTime, "%z", qArg(numToString(getTimeStamp()).substr(10, 3), 3));

	return strTime;
#endif // _WIN32
}

//时间戳转字符
std::string timeStampToString(long long time, const std::string& format)
{
	long long milli = time + (long long)8 * 60 * 60 * 1000;//此处转化为东八区北京时间，如果是其它时区需要按需求修改
	auto mTime = std::chrono::milliseconds(milli);
	auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
	time_t timeNow = std::chrono::system_clock::to_time_t(tp);
	struct tm* tmInfo = std::gmtime(&timeNow);
	std::string strTime = format;
	stringReplace(strTime, "%Y", numToString(tmInfo->tm_year + 1900));
	stringReplace(strTime, "%M", qArg(tmInfo->tm_mon + 1, 2));
	stringReplace(strTime, "%D", qArg(tmInfo->tm_mday, 2));
	stringReplace(strTime, "%h", qArg(tmInfo->tm_hour, 2));
	stringReplace(strTime, "%m", qArg(tmInfo->tm_min, 2));
	stringReplace(strTime, "%s", qArg(tmInfo->tm_sec, 2));
	std::string strTimeStamp = numToString(time);
	if (strTimeStamp.size() > 10)
		stringReplace(strTime, "%z", qArg(strTimeStamp.substr(10, strTimeStamp.size()), 3));
	else
		stringReplace(strTime, "%z", "000");

	return strTime;
}

//字符串时间转换时间
time_t stringToTimet(const std::string& strTime)
{
	tm tm;
	tm.tm_year = 0;                 // 年 
	tm.tm_mon = 0;                    // 月 tm结构体的月份存储范围为0-11
	tm.tm_mday = 0;                         // 日
	tm.tm_hour = 0;                        // 时
	tm.tm_min = 0;                       // 分
	tm.tm_sec = 0;                       // 秒
	tm.tm_isdst = 0;  // 非夏令时
	std::string time = stringReplaceRegex(strTime, "/", "-");
	time = stringReplaceRegex(time, "T", " ");
	time = stringReplaceRegex(time, "Z", "");
	if (sscanf(time.c_str(), "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min,
		&tm.tm_sec) != 0) {
		tm.tm_year -= 1900;  //tm结构体存储的是从1900年开始的时间
		tm.tm_mon -= 1;   //tm结构体的月份存储范围为0-11
	}
	time_t timet = mktime(&tm);                  // 将tm结构体转换成time_t格式。
	return timet;
}

//字符正则匹配 返回匹配结果 如果匹配成功不为空 列表第一个为全字匹配结果，第2个开始才为条件结果
std::vector<std::string> stringFindRegex(const std::string& data, const std::string& regex,
	std::regex_constants::match_flag_type flgs)
{
	std::vector<std::string> list;
	try {
		std::regex re(regex);
		std::smatch sm;
		size_t curPos = 0;
		while (std::regex_search(data.begin() + curPos, data.end(), sm, re, flgs)) {
			size_t pos = sm.position();
			for (size_t i = 0; i < sm.size(); i++) {
				list.push_back(sm[i]);
			}
			if (!sm.empty()) {
				pos += sm[0].str().size();
			}
			curPos += pos;
		}
	}
	catch (const std::regex_error& /*err*/) {
		//std::cout << err.what() << std::endl;
	}


	return list;
}

//字符正则替换
std::string stringReplaceRegex(const std::string& data, const std::string& regex, const std::string& after,
	std::regex_constants::match_flag_type flgs)
{
	try {
		std::regex re(regex);
		return std::regex_replace(data, re, after, flgs);
	}
	catch (const std::regex_error& /*err*/) {
		//std::cout << err.what() << std::endl;
	}

	return data;
}

//字符正则替换
std::string stringReplaceRegex(const std::string& data, const std::string& regex, const std::string& after, int replaceNum, std::regex_constants::match_flag_type flgs)
{
	std::string str = data;
	try {
		std::regex re(regex);
		std::smatch sm;
		size_t curPos = 0;
		int curReplaceNum = 0;
		while (std::regex_search(str.cbegin() + curPos, str.cend(), sm, re, flgs)) {
			size_t pos = sm.position();
			if (!sm.empty()) {
				str.replace(pos, sm[0].str().length(), after);
				if (replaceNum != 0 && ++curReplaceNum >= replaceNum) {
					break;
				}
				pos += sm[0].str().size();
			}
			curPos += pos;
		}
	}
	catch (const std::regex_error& /*err*/) {
		//std::cout << err.what() << std::endl;
	}

	return str;
}

//字符正则查询 返回查到的开始位置
int stringFindPosRegex(const std::string& data, unsigned int startPos, const std::string& regex, std::regex_constants::match_flag_type flgs)
{
	std::regex re(regex);
	std::smatch sm;
	int curPos = -1;
	if (std::regex_search(data.begin() + startPos, data.end(), sm, re, flgs)) {
		numToNum(sm.position(), curPos);
	}

	return curPos;
}

//std::string转wchar_t
wchar_t* stringToWChar(const std::string& data)
{
	std::wstring str = stringToWstring(data);
	wchar_t* charData = new wchar_t[str.size() + 1];
	zeroMemory(charData, sizeof(charData));
	wmemcpy(charData, str.c_str(), str.size());
	charData[str.size()] = '\0';
	return charData;
}

//判断路径文件是否存在
bool isFilePathExist(const std::string& filePath)
{
	std::fstream file;
	file.open(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}
	file.close();
	return true;
}

//创建文件夹
bool createDirectory(const std::string& path)
{
	std::vector<std::string> pathList = stringSplitRegex(path, "/+|\\+");
	if (pathList.empty())
		return false;

	std::string curPath;
	for (size_t i = 0; i < pathList.size(); i++) {
		if (!curPath.empty())
			curPath += "/";
		curPath += pathList.at(i);
		if (access(curPath.c_str(), 0) != 0) {
#ifdef _WIN32
			if (mkdir(curPath.c_str()) == -1) {
				return false;
			}
#elif __APPLE__ || __linux__ 
			if (mkdir(curPath.c_str(), 0777) == -1) {
				return false;
			}
#endif // _WIN32
		}
	}
	return true;
}

//删除文件
bool removeFilePath(const std::string& filePath)
{
	if (!isFilePathExist(filePath)) {
		return true;
	}

	if (remove(filePath.c_str()) != 0) {
		return false;
	}

	return true;
}

//清空数组内存置\0
void zeroMemory(void* des, std::size_t size)
{
	memset(des, '\0', size);
}

//暂停等待
void sleepTime(unsigned int time)
{
#ifdef _WIN32
	Sleep(time);
#elif __APPLE__ || __linux__
	usleep(time * 1000);
#endif // _WIN32
}

//复制文件
bool copyFile(const std::string& filePath, const std::string& newFilePath)
{
	if (!isFilePathExist(filePath)) {
		return false;
	}

	std::vector<std::string> pathList = stringSplitRegex(newFilePath, "/+|\\\\+");
	if (pathList.empty()) {
		return false;
	}

	if (pathList.size() > 2 || (pathList.size() == 2 && stringFindPosRegex(pathList.at(0), 0, "\\S:|.") == -1)) {
		std::string curPath;
		for (size_t i = 0; i < pathList.size() - 1; i++) {
			if (!curPath.empty()) {
				curPath += "/";
			}
			curPath += pathList.at(i);
		}

		if (!createDirectory(curPath)) {
			return false;
		}
	}

	if (!removeFilePath(newFilePath)) {
		return false;
	}

	std::fstream fileIn;
	fileIn.open(filePath, std::ios::in | std::ios::binary);
	if (!fileIn.is_open()) {
		return false;
	}

	std::fstream fileOut;
	fileOut.open(newFilePath, std::ios::out | std::ios::binary);
	if (!fileOut.is_open()) {
		fileIn.close();
		return false;
	}

	fileOut << fileIn.rdbuf();

	fileIn.close();
	fileOut.close();

	return true;
}

//读取文件
bool readFile(const std::string filePath, std::string& fileData) {

	std::fstream file;
	file.open(filePath, std::ios::in);
	if (!file.is_open()) {
		fileData = qstringArg("文件打开失败 %1", filePath);
		return false;
	}
	std::ostringstream tmp;
	tmp << file.rdbuf();
	fileData = tmp.str();  //保存所有文件内容
	file.close();
	return true;
}

//保存文件
bool writeFile(const std::string filePath, const std::string& fileData)
{
	std::vector<std::string> pathList = stringSplitRegex(filePath, "/+|\\+");
	if (pathList.empty()) {
		return false;
	}

	if (pathList.size() > 2 || (pathList.size() == 2 && stringFindPosRegex(pathList.at(0), 0, "\\S:|.") == -1)) {
		std::string curPath;
		for (size_t i = 0; i < pathList.size() - 1; i++) {
			if (!curPath.empty()) {
				curPath += "/";
			}
			curPath += pathList.at(i);
		}

		if (!createDirectory(curPath)) {
			return false;
		}
	}

	if (!removeFilePath(filePath)) {
		return false;
	}

	//写入文件
	std::fstream file;
	file.open(filePath, std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		return false;
	}
	file << fileData;
	file.close();
	return true;
}

//获取线程ID
unsigned long getThreadId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#elif __linux__
	return gettid();
#elif __APPLE__
	return 0;
#endif // _WIN32
}

//获取进程ID
unsigned long getProcessId()
{
#ifdef _WIN32
	return GetCurrentProcessId();
#elif __APPLE__ || __linux__
	return getpid();
#endif // _WIN32
}

//执行system命令 并返回结果
bool CommandSystem(const std::string& command, std::string& retData)
{
#ifdef _WIN32
	auto fp = _popen(command.c_str(), "r");
#elif __APPLE__ || __linux__
	auto fp = popen(command.c_str(), "r");
#endif // _WIN32
	if (fp == nullptr) {
		return false;
	}

	char data[2048];
	zeroMemory(data, sizeof(data));
	retData.clear();
	while (fgets(data, sizeof(data), fp) != NULL) {
		retData.append(data, strlen(data));
	}
#ifdef _WIN32
	if (_pclose(fp) == -1) {
		return false;
	}
#elif __APPLE__ || __linux__
	if (pclose(fp) == -1) {
		return false;
	}
#endif // _WIN32

	return true;
}

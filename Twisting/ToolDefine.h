#pragma once
/*
工具函数 Mr.lei
所有函数支持跨平台
*/
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <list>
#include <regex>
#include <qregexp.h>
#include <qstring.h>
#include <qlist.h>

//字符到int
int fromHex(char ch);

//Url编码
std::string urlEncode(const std::string& str);

//Url解码
//plusAsSpace 为true表示替换未编码的+为空格
std::string urlDecode(const std::string& str, bool plusAsSpace = false);

//16进制字符串到字符
//算法来源QByteArray::fromHex
std::string fromHex(const std::string& str);

//HEX转字符串 0不转换字符大小写 1转换为小写 2转换为大写
std::string hexToString(const std::string& str, int type = 0, const std::string& prefix = "");

//获取目录文件
void getPathFiles(std::string path, std::list<std::string>& files);

//通过正则匹配目录中文件
void getFiles(QString path, QList<QString>& files, QRegExp re);

//字符替换
//replaceNum为替换数量 默认替换全部
std::string stringReplace(const std::string& str, const std::string& before, const std::string& after, int replaceNum = 0);

//字符串查询替换 
//replaceNum为替换数量 默认替换全部
void stringReplace(std::string& str, const std::string& before, const std::string& after, int replaceNum = 0);

//字符串分割
std::vector<std::string> stringSplit(const std::string& str, const std::string& delim);

//字符串分割 正则
std::vector<std::string> stringSplitRegex(const std::string& str, const std::string& regex,
	std::regex_constants::match_flag_type flgs = std::regex_constants::match_default);

//获取程序工作路径
void getWorkPath(std::string& path);

//获取程序工作路径
std::string getWorkPath();

//获取程序文件绝对路径
void getAbsolutePath(std::string& path);

//获取程序文件绝对路径
std::string getAbsolutePath();

//UTF_8转GBK
std::string utf8ToGbk(const std::string& utf8String);

//GBK转UTF-8
std::string gbkToUtf8(const std::string& gbkString);

//string转 wstring
std::wstring stringToWstring(const std::string& data);

//wstring转string
std::string wstringToString(const std::wstring& data);

//获取时间戳 毫秒
long long getTimeStamp();

//获取当前时间
//格式取任意值 %Y-%M-%D %h:%m:%s:%z
std::string getCurTime(const std::string& format);

//时间戳转字符
//%Y-%M-%D %h:%m:%s:%z
std::string timeStampToString(long long time, const std::string& format);

//字符串时间转换时间
//%Y-%M-%D %h:%m:%s:%z
time_t stringToTimet(const std::string& strTime);

//字符正则匹配 返回匹配结果 如果匹配成功不为空 列表第一个为全字匹配结果，第2个开始才为条件结果
std::vector<std::string> stringFindRegex(const std::string& data, const std::string& regex,
	std::regex_constants::match_flag_type flgs = std::regex_constants::match_default);

//字符正则替换
std::string stringReplaceRegex(const std::string& data, const std::string& regex, const std::string& after,
	std::regex_constants::match_flag_type flgs = std::regex_constants::match_default);

//字符正则替换
//replaceNum为替换数量 默认替换全部
std::string stringReplaceRegex(const std::string& data, const std::string& regex, const std::string& after,
	int replaceNum, std::regex_constants::match_flag_type flgs = std::regex_constants::match_default);

//字符正则查询 返回查到的开始位置 未找到返回-1
int stringFindPosRegex(const std::string& data, unsigned int startPos, const std::string& regex,
	std::regex_constants::match_flag_type flgs = std::regex_constants::match_default);

//std::string转wchar_t
wchar_t* stringToWChar(const std::string& data);

//判断路径文件是否存在
bool isFilePathExist(const std::string& filePath);

//创建文件夹
bool createDirectory(const std::string& path);

//删除文件
bool removeFilePath(const std::string& filePath);

//清空数组内存置\0
void zeroMemory(void* des, std::size_t size);

//暂停等待 毫秒
void sleepTime(unsigned int time);

//复制文件
bool copyFile(const std::string& filePath, const std::string& newFilePath);

//读取文件
bool readFile(const std::string filePath, std::string& fileData);

//保存文件
bool writeFile(const std::string filePath, const std::string& fileData);

//获取线程ID
unsigned long getThreadId();

//获取进程ID
unsigned long getProcessId();

//执行system命令 并返回结果
bool CommandSystem(const std::string& command, std::string& retData);

//任意类型转字符串 n表示截取精度
template <typename T>
std::string numToString(const T num, int n = 6)
{
	std::stringstream out;
	out << std::setprecision(n) << std::setiosflags(std::ios::fixed) << num;
	return out.str();
}

//字符串转数值数据类型
template <typename T>
void stringToNum(const std::string& str, T& num)
{
	std::stringstream out;
	out << str;
	out >> num;
}

//字符串转数值数据类型
//使用方式 double num = stringToNum<double>("1.23");
template<typename T>
T stringToNum(const std::string& str)
{
	T t{};
	std::stringstream out;
	out << str;
	out >> t;
	return t;
}

//任意数值数据类型互相转换 n表示截取精度
template <typename TIN, typename TOUT>
void numToNum(TIN num, TOUT& toNum, int n = 6)
{
	std::stringstream out;
	out << std::setprecision(n) << std::setiosflags(std::ios::fixed) << num;
	out >> toNum;
}

//任意数值数据类型互相转换 n表示截取精度
//使用方式 int num = numToNum<double, int>(1.23);
template <typename TIN, typename TOUT>
TOUT numToNum(TIN num, int n = 6)
{
	TOUT t{};
	std::stringstream out;
	out << std::setprecision(n) << std::setiosflags(std::ios::fixed) << num;
	out >> t;
	return t;
}

//字符串格式化替换 类似CString 此函数是不安全的注意形参个数和替换类型 不匹配会崩 不建议使用
template <typename... Args>
std::string stringArg(const char* format, Args... args)
{
	int size = std::snprintf(nullptr, 0, format, args...);
	if (size <= 0) {
		return "";
	}
	size += 1;
	char* szBuf = new char[size];
	std::snprintf(szBuf, size, format, args...);

	std::string str(szBuf);
	delete[] szBuf;
	return str;
}

//退化类型 判断模板形参类型
template <typename T, typename U>
struct decayName :
	std::is_same<typename std::decay<T>::type, U>::type
{};

//展开变长形参 替换格式化内容 请不要直接调用这个函数
template <typename T>
void unFormatArgs(char* szBuf, unsigned int size, const T& data, int argsSize, int& curArgsSize)
{
	std::string strBuf(szBuf, strlen(szBuf));
	if (curArgsSize == 0)
		stringReplace(strBuf, "%%", "{}");
	zeroMemory(szBuf, size);
	memcpy(szBuf, strBuf.c_str(), strBuf.size());
	int curNum = 0;
	unsigned int curPos = 0;
	unsigned int lastPos = 0;
	unsigned int findPos = 0;
	while ((curPos = stringFindPosRegex(strBuf, findPos, "%(?!%)")) != -1) {
		if (curNum == 1)
			lastPos += 1;
		lastPos += curPos;
		findPos += curPos + 1;
		if (++curNum == 2) {
			break;
		}
	}
	std::string format;
	if (curNum == 2) {
		format.append(szBuf, szBuf + lastPos);
	}
	else {
		format.append(szBuf, strlen(szBuf));
	}
	zeroMemory(szBuf, size);

	if (decayName<T, std::string>::value) {
		std::stringstream str;
		str << data;
		std::snprintf(szBuf, size, format.c_str(), str.str().c_str());
	}
	else {
		std::snprintf(szBuf, size, format.c_str(), data);
	}

	std::string strData(szBuf, strlen(szBuf));
	if (++curArgsSize == argsSize)
		stringReplace(strData, "{}", "%");
	if (curNum == 2)
		strData.append(strBuf.begin() + lastPos, strBuf.end());
	zeroMemory(szBuf, size);
	memcpy(szBuf, strData.c_str(), strData.size());
}

//展开变长形参 计算大小 请不要直接调用这个函数
template <typename T>
void unFormatSizeArgs(const T& data, unsigned int& size)
{
	std::stringstream str;
	str << data;
	unsigned int strSize = 0;
	numToNum(str.str().size(), strSize);
	size += strSize;
}

//字符串格式化替换 类似CString 此函数是不安全的注意形参个数和替换类型 不匹配会崩 不建议使用
//此函数比起stringArg区别是std::string不用自己调用.c_str()
template <typename... Args>
std::string stringFormat(const char* format, Args... args)
{
	int argsSize = sizeof...(args);
	unsigned int strSize = 0;
	struct dummy {
		dummy(std::initializer_list<int>) {}
	};
	//先展开变长形参 计算需求长度
	dummy{ (unFormatSizeArgs<Args>(std::forward<Args>(args), strSize), 0)... };
	unsigned int formatSize = 0;
	numToNum(strlen(format), formatSize);
	strSize += formatSize + 255;  //加上预留值 有些特殊格式化
	char* szBuf = new char[strSize];
	zeroMemory(szBuf, strSize);
	memcpy(szBuf, format, strlen(format));
	int curArgsSize = 0;
	dummy{ (unFormatArgs<Args>(szBuf, strSize, std::forward<Args>(args), argsSize, curArgsSize), 0)... };

	std::string str(szBuf);
	delete[] szBuf;
	return str;
}

//展开变长形参替换 请不要直接调用这个函数
template <typename T>
void unArgs(std::string& format, const T& data, std::vector<std::string>& list)
{
	std::stringstream out;
	out << data;
	list.push_back(out.str());
}

//qstringArg函数功能扩展 5个形参分别是 源数据 最小宽度 填充内容 对齐方式(0默认 1左对齐 2右对齐) 浮点数精度
//使用方式 qstringArg("%1", qArg(1.3, 3, 0));
//功能借鉴QString().arg()实现重载功能比较多暂时只实现这个后面不足再补充
template <typename T>
std::string qArg(const T& data, int minWidth = 0, char fillData = '0', int align = 0, int precision = -1)
{
	std::stringstream out;
	out.width(minWidth);
	out.fill(fillData);
	if (decayName<T, double>::value || decayName<T, float>::value) {
		if (precision >= 0)
			out << std::setprecision(precision);
		out << std::setiosflags(std::ios::fixed);
	}
	if (align == 1) {
		out << std::setiosflags(std::ios::left);
	}
	else if (align == 2) {
		out << std::setiosflags(std::ios::right);
	}
	out << data;
	return out.str();
}

//字符串格式化替换 类似QString("%1%2").arg(1).arg(2)  //此函数是安全的不用考虑形参类型和数量
//使用方式 qstringArg("%1 test %2", "hello", 1.234);  //可以同一形参替换多个位置
template <typename... Args>
std::string qstringArg(const char* format, Args... args)
{
	//int size = sizeof...(args);
	std::vector<std::string> list;
	std::string data(format);
	//auto f = [](...) {};  //Lambda表达式 clang++编译器是正序弃用此倒序方式
	//f((unArgs<Args>(data, std::forward<Args>(args), size), 0)...);  //std::forward 完美转发传递参数
	struct dummy {
		dummy(std::initializer_list<int>) {}
	};
	dummy{ (unArgs<Args>(data, std::forward<Args>(args), list), 0)... };
	for (size_t i = list.size(); i > 0; i--) {
		std::string rep = stringArg("%%%d", i);
		stringReplace(data, rep, list.at(i - 1));
	}
	return data;
}

//设置指定位的值 输入值 输出值 切第几位 位值 注意不要超出unsigned long long限制
template <typename TIN, typename TOUT>
void setBitValue(const TIN& in, TOUT& out, int num, bool value)
{
	if (value) {
		out = in | ((unsigned long long)1 << num);
	}
	else {
		out = in & ~((unsigned long long)1 << num);
	}
}

//读取位值
template <typename T>
bool readBitValue(const T& in, int num)
{
	bool value = true;
	T temp = in;
	if ((1 & (temp >> num)) == 1)
		value = true;
	else
		value = false;

	return value;
}

//补码4字节转为数值
template <typename TIN, typename TOUT>
void complementCodeNumToNum(const TIN& data, TOUT& num)
{
	std::stringstream ss;
	ss << std::hex << data;
	int x = 0;
	ss >> x;
	ss.clear();
	if (readBitValue(x, 0)) {  //判断是否为复数 复数得用无符号不然计算溢出
		unsigned int y = 0;
		ss << std::hex << data;
		ss >> y;
		y = y - 1;
		y = ~y;
		ss.clear();
		ss << (float)y;
	}
	else {
		ss << (float)x;
	}

	ss >> num;
	if (readBitValue(x, 0)) {
		num = -num;
	}
}


//4字节16进制转为数值
template <typename TIN, typename TOUT>
void hexStringToNum(const TIN& str, TOUT& num)
{
	std::stringstream ss;
	ss << std::hex << str;
	unsigned int x = 0;
	ss >> x;
	ss.clear();
	ss << reinterpret_cast<float&>(x);
	ss >> num;
}

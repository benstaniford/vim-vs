#include "vimvsPCH.h"
#include "Utils.h"
#include "Logging.h"
#include "ScopeGuard.h"

namespace cz
{

std::wstring getWin32Error(const wchar_t* funcname)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	               NULL,
	               dw,
	               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	               (LPTSTR)&lpMsgBuf,
	               0,
	               NULL);
	SCOPE_EXIT{ LocalFree(lpMsgBuf); };

	int funcnameLength = funcname ? lstrlen((LPCTSTR)funcname) : 0;
	lpDisplayBuf =
	    (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + funcnameLength + 50) * sizeof(funcname[0]));
	if (lpDisplayBuf == NULL)
		return L"Win32ErrorMsg failed";
	SCOPE_EXIT{ LocalFree(lpDisplayBuf); };

	StringCchPrintfW((LPTSTR)lpDisplayBuf,
	                 LocalSize(lpDisplayBuf) / sizeof(funcname[0]),
	                 TEXT("%s failed with error %lu: %s"),
	                 funcname ? funcname : L"",
	                 dw,
	                 (LPTSTR)lpMsgBuf);

	std::wstring ret = (LPTSTR)lpDisplayBuf;

	// Remove the \r\n at the end
	while (ret.size() && ret.back() < ' ')
		ret.pop_back();

	return std::move(ret);
}

void _doAssert(const wchar_t* file, int line, _Printf_format_string_ const wchar_t* fmt, ...)
{
	static bool executing;

	// Detect reentrancy, since we call a couple of things from here, that might end up asserting
	if (executing)
		__debugbreak();
	executing = true;

	wchar_t buf[1024];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf(buf, 1024, fmt, args);
	va_end(args);

	CZ_LOG(logDefault, Fatal, L"ASSERT: %s,%d: %s\n", file, line, buf);

	if (::IsDebuggerPresent())
	{
		__debugbreak(); // This will break in all builds
	}
	else
	{
		//_wassert(wbuf, wfile, line);
		//DebugBreak();
		__debugbreak(); // This will break in all builds
	}
}

wchar_t* getTemporaryString()
{
	// Use several static strings, and keep picking the next one, so that callers can hold the string for a while
	// without risk of it being changed by another call.
	__declspec(thread) static wchar_t bufs[kTemporaryStringMaxNesting][kTemporaryStringMaxSize];
	__declspec(thread) static wchar_t nBufIndex = 0;
	wchar_t* buf = bufs[nBufIndex];
	nBufIndex++;
	if (nBufIndex == kTemporaryStringMaxNesting)
		nBufIndex = 0;
	return buf;
}

const wchar_t* formatString(_Printf_format_string_ const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	const wchar_t* str = formatStringVA(format, args);
	va_end(args);
	return str;
}

const wchar_t* formatStringVA(const wchar_t* format, va_list argptr)
{
	wchar_t* buf = getTemporaryString();
	_vsnwprintf_s(buf, kTemporaryStringMaxSize, _TRUNCATE, format, argptr);
	return buf;
}

void ensureTrailingSlash(std::wstring& str)
{
	if (str.size() && !(str[str.size() - 1] == '\\' || str[str.size() - 1] == '/'))
		str += '\\';
}

std::wstring getCWD()
{
	const int bufferLength = MAX_PATH;
	wchar_t buf[bufferLength + 1];
	buf[0] = 0;
	CZ_CHECK(GetCurrentDirectoryW(bufferLength, buf) != 0);
	std::wstring res = buf;
	return res + L"\\";
}

bool isExistingFile(const std::wstring& filename)
{
	DWORD dwAttrib = GetFileAttributesW(filename.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring getProcessPath(std::wstring* fname)
{
	wchar_t buf[MAX_PATH];
	GetModuleFileNameW(NULL, buf, MAX_PATH);

	std::wstring result(buf);
	std::wstring::size_type index = result.rfind(L"\\");

	if (index != std::wstring::npos)
	{
		if (fname)
			*fname = result.substr(index + 1);
		result = result.substr(0, index + 1);
	}
	else
		return L"";

	return result;
}

bool fullPath(std::wstring& dst, const std::wstring& path, std::wstring root)
{
	wchar_t fullpathbuf[MAX_PATH + 1];
	wchar_t srcfullpath[MAX_PATH + 1];
	if (root.empty())
		root = getCWD();
	ensureTrailingSlash(root);

	std::wstring tmp = PathIsRelativeW(path.c_str()) ? root + path : path;
	wcscpy(srcfullpath, tmp.c_str());
	wchar_t* d = srcfullpath;
	wchar_t* s = srcfullpath;
	while (*s)
	{
		if (*s == '/')
			*s = '\\';
		*d++ = *s;

		// Skip any repeated separator
		if (*s == '\\')
		{
			s++;
			while (*s && (*s == '\\' || *s == '/'))
				s++;
		}
		else
		{
			s++;
		}
	}
	*d = 0;

	bool res = PathCanonicalizeW(fullpathbuf, srcfullpath) ? true : false;
	if (res)
		dst = fullpathbuf;
	return res;
}

std::wstring replace(const std::wstring& s, wchar_t from, wchar_t to)
{
	std::wstring res = s;
	for (auto&& ch : res)
	{
		if (ch == from)
			ch = to;
	}
	return res;
}

std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	if (from.empty())
		return L"";
	size_t start_pos = 0;
	auto res = str;
	while ((start_pos = res.find(from, start_pos)) != std::string::npos)
	{
		res.replace(start_pos, from.length(), to);
		start_pos += to.length();  // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
	return res;
}

std::pair<std::wstring, std::wstring> splitFolderAndFile(const std::wstring& str)
{
	auto i = std::find_if(str.rbegin(), str.rend(), [](const wchar_t& ch)
	{
		return ch == '/' || ch == '\\';
	});

	std::pair < std::wstring, std::wstring> res;
	res.first = std::wstring(str.begin(), i.base());
	res.second = std::wstring(i.base(), str.end());
	return res;
}

std::wstring toUTF16(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int length = ::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		NULL,                // unused - no conversion done in this step
		0                    // request size of destination buffer, in wchar_t's
	);
	if (length == 0)
		throw std::exception("Can't get length of UTF-16 string");

	std::wstring utf16;
	utf16.resize(length);

	// Do the actual conversion
	if (!::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		&utf16[0],           // destination buffer
		(int)utf16.length()  // size of destination buffer, in wchar_t's
	))
	{
		throw std::exception("Can't convert string from UTF-8 to UTF-16");
	}

	return utf16;
}

std::string toUTF8(const std::wstring& utf16)
{
	if (utf16.empty())
		return std::string();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int utf8_length = ::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
		0,                    // default flags
		utf16.data(),         // source UTF-16 string
		(int)utf16.length(),  // source string length, in wchar_t's,
		NULL,                 // unused - no conversion required in this step
		0,                    // request buffer size
		NULL,
		NULL  // unused
	);

	if (utf8_length == 0)
		throw "Can't get length of UTF-8 string";

	std::string utf8;
	utf8.resize(utf8_length);

	// Do the actual conversion
	if (!::WideCharToMultiByte(CP_UTF8,              // convert to UTF-8
		0,                    // default flags
		utf16.data(),         // source UTF-16 string
		(int)utf16.length(),  // source string length, in wchar_t's,
		&utf8[0],             // destination buffer
		(int)utf8.length(),   // destination buffer size, in chars
		NULL,
		NULL  // unused
	))
	{
		throw "Can't convert from UTF-16 to UTF-8";
	}

	return utf8;
}

bool isSpace(int a)
{
	return a == ' ' || a == '\t' || a == 0xA || a == 0xD;
}

bool notSpace(int a)
{
	return !isSpace(a);
}

std::wstring widen(const std::string& utf8)
{
	if (utf8.empty())
		return std::wstring();

	// Get length (in wchar_t's), so we can reserve the size we need before the
	// actual conversion
	const int length = ::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		NULL,                // unused - no conversion done in this step
		0                    // request size of destination buffer, in wchar_t's
	);
	if (length == 0)
		throw std::exception("Can't get length of UTF-16 string");

	std::wstring utf16;
	utf16.resize(length);

	// Do the actual conversion
	if (!::MultiByteToWideChar(CP_UTF8,             // convert from UTF-8
		0,                   // default flags
		utf8.data(),         // source UTF-8 string
		(int)utf8.length(),  // length (in chars) of source UTF-8 string
		&utf16[0],           // destination buffer
		(int)utf16.length()  // size of destination buffer, in wchar_t's
	))
	{
		throw std::exception("Can't convert string from UTF-8 to UTF-16");
	}

	return utf16;
}

bool endsWith(const std::wstring& str, const std::wstring& ending)
{
	if (str.length() >= ending.length()) {
		return (0 == str.compare(str.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

bool endsWith(const std::wstring& str, const wchar_t* ending)
{
	const size_t endingLength = wcslen(ending);
	if (str.length() >= endingLength) {
		return (0 == str.compare(str.length() - endingLength, endingLength, ending));
	}
	else {
		return false;
	}
}

bool beginsWith(const std::wstring& str, const std::wstring& begins)
{
	if (str.length() >= begins.length()) {
		return (0 == str.compare(0, begins.length(), begins));
	}
	else {
		return false;
	}
}

bool beginsWith(const std::wstring& str, const wchar_t* begins)
{
	const size_t beginsLength = wcslen(begins);
	if (str.length() >= beginsLength) {
		return (0 == str.compare(0, beginsLength, begins));
	}
	else {
		return false;
	}
}

}
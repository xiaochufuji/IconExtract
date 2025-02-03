#include "IconExtract.h"

int main() {
	xiaochufuji::IconExtract icon(LR"(D:\weixin\WeChat\WeChat.exe)", 256);
	xiaochufuji::CreateDirectoryIfNotExists(L"img");
	if (icon.saveAll(L"./img/wechat", xiaochufuji::IconFormat::PNG))
		std::cout << "success" << std::endl;
	icon.reload(LR"(C:\Program Files\Google\Chrome\Application\chrome.exe)", 256);
	if (icon.saveAll(L"./img/chrome", xiaochufuji::IconFormat::PNG))
		std::cout << "success" << std::endl;
	return 0;
}

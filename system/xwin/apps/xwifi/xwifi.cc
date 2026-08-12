#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/List.h>
#include <Widget/EditLine.h>
#include <Widget/LabelButton.h>
#include <Widget/Scroller.h>
#include <Widget/Splitter.h>
#include <Widget/Blank.h>

#include <x++/X.h>
#include <ewoksys/vdevice.h>
#include <tinyjson/tinyjson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>

using namespace Ewok;

static const char* WLAN_DEV = "/dev/wl0";
static const char* NET_DEV = "/dev/net0";

static bool macEquals(const std::string& a, const std::string& b) {
	if(a.empty() || b.empty() || a.size() != b.size())
		return false;
	for(size_t i=0; i<a.size(); i++) {
		if(tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
			return false;
	}
	return true;
}
static const uint32_t MAX_WIFI_ITEMS = 64;
static const uint32_t SCAN_WAIT_TICKS = 12;

struct WifiItem {
	std::string ssid;
	std::string bssid;
	std::string auth;
	std::string cipher;
	std::string type;
	int32_t rssi;
	int32_t channel;
	bool connected;

	WifiItem(): rssi(0), channel(0), connected(false) {
	}
};

class WifiWin;

class InfoPanel: public Widget {
	WifiWin* app;
	std::string text;

	void drawLine(graph_t* g, XTheme* theme, const grect_t& r, int32_t& y, const char* s) {
		if(s == NULL || s[0] == 0)
			return;
		graph_draw_text_font(g, r.x+8, y, s, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
		y += (int32_t)theme->basic.fontSize + 4;
	}
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);

		const char* p = text.c_str();
		const char* start = p;
		int32_t y = r.y + 8;
		char line[256];

		while(*p != 0) {
			if(*p == '\n') {
				int32_t len = (int32_t)(p - start);
				if(len > 0) {
					if(len >= (int32_t)sizeof(line))
						len = sizeof(line) - 1;
					memcpy(line, start, len);
					line[len] = 0;
					drawLine(g, theme, r, y, line);
				}
				p++;
				start = p;
				if(y >= (r.y + r.h - ((int32_t)theme->basic.fontSize + 4)))
					return;
				continue;
			}
			p++;
		}

		if(p != start) {
			int32_t len = (int32_t)(p - start);
			if(len >= (int32_t)sizeof(line))
				len = sizeof(line) - 1;
			memcpy(line, start, len);
			line[len] = 0;
			drawLine(g, theme, r, y, line);
		}
	}

	void onTimer(uint32_t timerFPS, uint32_t timerSteps);
public:
	InfoPanel(WifiWin* app): app(app) {
	}

	void setText(const std::string& s) {
		if(text == s)
			return;
		text = s;
		update();
	}
};

/*map rssi(dBm) to 0..4 bars of the classic signal icon*/
static int32_t rssiLevel(int32_t rssi) {
	if(rssi == 0)
		return 0;
	if(rssi >= -50)
		return 4;
	if(rssi >= -60)
		return 3;
	if(rssi >= -70)
		return 2;
	return 1;
}

class WifiList: public List {
	WifiWin* app;
	WifiItem items[MAX_WIFI_ITEMS];
	uint32_t count;
	std::string emptyText;

	/*classic stepped signal bars, always pinned to the row's right edge
	  so every row's icon lines up regardless of ssid length*/
	void drawSignalIcon(graph_t* g, XTheme* theme, const grect_t& r, int32_t level, uint32_t fg) {
		const int32_t bars = 4;
		const int32_t barW = 3;
		const int32_t gap = 2;
		const int32_t step = 3;
		int32_t totalW = bars * barW + (bars - 1) * gap;
		int32_t baseY = r.y + r.h - 5;
		int32_t left = r.x + r.w - 6 - totalW;

		for(int32_t i = 0; i < bars; i++) {
			int32_t bh = (i + 1) * step + 1;
			int32_t x = left + i * (barW + gap);
			graph_fill_rect(g, x, baseY - bh, barW, bh,
					(i < level) ? fg : theme->basic.fgDisableColor);
		}
	}

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);
		if(count == 0) {
			graph_draw_text_font(g, r.x+8, r.y+8,
				emptyText.c_str(),
				theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
		}
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index < 0 || (uint32_t)index >= count)
			return;

		const WifiItem& item = items[index];
		uint32_t fg = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			fg = theme->basic.selectColor;
		}

		char line[160];
		snprintf(line, sizeof(line), "%c %s",
				item.connected ? '*' : ' ',
				item.ssid.c_str());
		graph_draw_text_font(g, r.x+4, r.y+4, line, theme->getFont(), theme->basic.fontSize, fg);
		drawSignalIcon(g, theme, r, rssiLevel(item.rssi), fg);
	}

	void onSelect(int32_t index);

public:
	WifiList(WifiWin* app): app(app), count(0) {
		emptyText = "Scanning Wi-Fi...";
	}

	void setItems(const WifiItem* src, uint32_t num, const std::string& selectedSsid);
	void setEmptyText(const std::string& s) {
		emptyText = s;
		update();
	}
	const WifiItem* getItem(int32_t index) const {
		if(index < 0 || (uint32_t)index >= count)
			return NULL;
		return &items[index];
	}

	uint32_t getCount(void) const {
		return count;
	}
};

class WifiWin: public WidgetWin {
	WifiList* wifiList;
	EditLine* password;
	InfoPanel* infoPanel;
	std::string wlanState;
	std::string wlanSsid;
	std::string wlanMac;
	std::string wlanIp;
	std::string wlanBssid;
	std::string wlanAuth;
	std::string wlanCipher;
	int32_t wlanChannel;
	int32_t wlanRssi;
	bool scanReady;
	uint32_t scanWaitTicks;
	WifiItem wifiCache[MAX_WIFI_ITEMS];
	uint32_t wifiCacheNum;

public:
	WifiWin() {
		wifiList = NULL;
		password = NULL;
		infoPanel = NULL;
		wlanChannel = 0;
		wlanRssi = 0;
		scanReady = false;
		scanWaitTicks = 0;
		wifiCacheNum = 0;
	}

	static void connectClick(Widget* wd, xevent_t* evt, void* arg) {
		(void)wd;
		if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
			return;
		((WifiWin*)arg)->connectSelected();
	}

	static void refreshClick(Widget* wd, xevent_t* evt, void* arg) {
		(void)wd;
		if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
			return;
		((WifiWin*)arg)->manualRefresh();
	}

	void setWidgets(WifiList* list, EditLine* edit, InfoPanel* info) {
		wifiList = list;
		password = edit;
		infoPanel = info;
		wifiList->setEmptyText("Scanning Wi-Fi...");
	}

	void updateEmptyHint() {
		if(wifiList == NULL || wifiList->getCount() > 0)
			return;

		if(scanReady || scanWaitTicks == 0)
			wifiList->setEmptyText("No Wi-Fi found. Click refresh to rescan.");
		else
			wifiList->setEmptyText("Scanning Wi-Fi...");
	}

	void poll(uint32_t timerFPS, uint32_t timerSteps) {
		if(scanWaitTicks > 0) {
			scanWaitTicks--;
			if(scanWaitTicks == 0)
				updateEmptyHint();
		}

		if(timerSteps == 1) {
			/*one kick at startup; the driver auto-rescans every 5s while
			  disconnected, and the refresh button rescans on demand.
			  Periodic "scan" commands from here kept the firmware's mpc
			  toggling off/on and pinned wland's worker at 100% CPU.*/
			triggerScan();
			refreshList();
			refreshState();
			return;
		}

		/*state every 500ms so connecting->connected shows up promptly
		  (it is a cheap struct read in wland, no firmware access); the
		  list stays on the 2s cadence*/
		if((timerSteps % 2) == 0)
			refreshState();
		if((timerSteps % (timerFPS * 2)) == 0)
			refreshList();
	}

	void onSelected(int32_t index) {
		(void)index;
		updateInfo();
	}

	void manualRefresh() {
		triggerScan();
		refreshList();
		refreshState();
		updateInfo();
	}

	void connectSelected() {
		if(wifiList == NULL || password == NULL)
			return;

		const WifiItem* item = wifiList->getItem(wifiList->getSelected());
		if(item == NULL)
			return;

		char cmd[192];
		snprintf(cmd, sizeof(cmd), "connect %s %s",
				item->ssid.c_str(),
				password->getContent().c_str());
		char* ret = dev_cmd(WLAN_DEV, cmd);
		if(ret != NULL)
			free(ret);

		refreshState();
		refreshList();
	}

	void triggerScan() {
		char* ret = dev_cmd(WLAN_DEV, "scan");
		scanReady = false;
		scanWaitTicks = SCAN_WAIT_TICKS;
		updateEmptyHint();
		if(ret != NULL)
			free(ret);
		updateInfo();
	}

	void refreshList() {
		char* ret = dev_cmd(WLAN_DEV, "list");
		if(ret == NULL)
			return;

		json_var_t* arr = json_parse_str(ret);
		free(ret);
		if(arr == NULL)
			return;

		std::string selectedSsid;
		const WifiItem* curr = wifiList->getItem(wifiList->getSelected());
		if(curr != NULL)
			selectedSsid = curr->ssid;

		/*parse this poll's results, dedup by ssid keeping the strongest AP*/
		WifiItem incoming[MAX_WIFI_ITEMS];
		uint32_t incomingNum = 0;
		uint32_t num = json_var_array_size(arr);

		for(uint32_t i=0; i<num; i++) {
			json_var_t* item = json_var_array_get_var(arr, i);
			if(item == NULL)
				continue;

			WifiItem wifi;
			wifi.ssid = json_get_str_def(item, "ssid", "");
			wifi.bssid = json_get_str_def(item, "bssid", "");
			wifi.auth = json_get_str_def(item, "auth", "");
			wifi.cipher = json_get_str_def(item, "cipher", "");
			wifi.type = json_get_str_def(item, "type", "");
			wifi.rssi = json_get_int_def(item, "rssi", 0);
			wifi.channel = json_get_int_def(item, "channel", 0);
			wifi.connected = json_get_bool_def(item, "selected", false);

			if(wifi.ssid.empty())
				continue;

			int32_t existed = -1;
			for(uint32_t j=0; j<incomingNum; j++) {
				if(incoming[j].ssid == wifi.ssid) {
					existed = (int32_t)j;
					break;
				}
			}

			if(existed >= 0) {
				WifiItem& old = incoming[existed];
				/*same ssid: keep the strongest AP only; the connected
				  mark belongs to the ssid, so never lose it when a
				  stronger sibling wins*/
				bool connected = old.connected || wifi.connected;
				if(wifi.rssi > old.rssi)
					old = wifi;
				old.connected = connected;
			}
			else if(incomingNum < MAX_WIFI_ITEMS) {
				incoming[incomingNum++] = wifi;
			}
		}

		/*merge into the in-memory cache: known ssids are updated, new ones
		  appended, and entries missing from this response are kept as-is.
		  The driver's scan cache is empty while a connect runs, so without
		  this the list would blank out exactly when the user is watching
		  it.*/
		for(uint32_t i=0; i<incomingNum; i++) {
			int32_t slot = -1;
			for(uint32_t j=0; j<wifiCacheNum; j++) {
				if(wifiCache[j].ssid == incoming[i].ssid) {
					slot = (int32_t)j;
					break;
				}
			}
			if(slot >= 0)
				wifiCache[slot] = incoming[i];
			else if(wifiCacheNum < MAX_WIFI_ITEMS)
				wifiCache[wifiCacheNum++] = incoming[i];
		}

		/*the connected mark follows the polled wlan state, not the driver
		  list (which may not contain the active AP at all)*/
		for(uint32_t j=0; j<wifiCacheNum; j++) {
			wifiCache[j].connected = (!wlanSsid.empty() &&
					wifiCache[j].ssid == wlanSsid &&
					(wlanState == "connected" || wlanState == "connecting"));
		}

		wifiList->setItems(wifiCache, wifiCacheNum, selectedSsid);
		updateEmptyHint();
		json_var_unref(arr);
	}

	/*the wlan driver never knows its own ip (netd/dhcp owns it), so ask
	  netd and pick the interface whose mac matches the wlan mac*/
	std::string queryWlanIp() {
		if(wlanMac.empty())
			return "";

		char* ret = dev_cmd(NET_DEV, "ip");
		if(ret == NULL)
			return "";

		json_var_t* arr = json_parse_str(ret);
		free(ret);
		if(arr == NULL)
			return "";

		std::string ip;
		uint32_t num = json_var_array_size(arr);
		for(uint32_t i=0; i<num; i++) {
			json_var_t* it = json_var_array_get_var(arr, i);
			if(it == NULL)
				continue;
			if(macEquals(json_get_str_def(it, "mac", ""), wlanMac)) {
				ip = json_get_str_def(it, "ip", "");
				break;
			}
		}
		json_var_unref(arr);
		return ip;
	}

	void refreshState() {
		char* ret = dev_cmd(WLAN_DEV, "state");
		if(ret == NULL)
			return;

		json_var_t* obj = json_parse_str(ret);
		free(ret);
		if(obj == NULL)
			return;

		wlanState = json_get_str_def(obj, "state", "unknown");
		wlanSsid = json_get_str_def(obj, "ssid", "");
		wlanMac = json_get_str_def(obj, "mac", "");
		wlanBssid = json_get_str_def(obj, "bssid", "");
		wlanAuth = json_get_str_def(obj, "auth", "");
		wlanCipher = json_get_str_def(obj, "cipher", "");
		wlanChannel = json_get_int_def(obj, "channel", 0);
		wlanRssi = json_get_int_def(obj, "rssi", 0);

		wlanIp = json_get_str_def(obj, "ip", "");
		if(wlanIp.empty())
			wlanIp = queryWlanIp();

		scanReady = json_get_bool_def(obj, "scan_ready", false);
		if(scanReady)
			scanWaitTicks = 0;
		updateEmptyHint();

		json_var_unref(obj);
		updateInfo();
	}

	void updateInfo() {
		char line[192];
		std::string info = "WLAN Info\n";
		info += "----------------\n";

		/*the panel only ever shows four states: scanning, connecting,
		  getting ip, connected. idle/disconnected means the driver is
		  auto-scanning; connected without an ip yet means DHCP is
		  still running.*/
		const char* shownState = "scanning";
		if(wlanState == "connecting")
			shownState = "connecting";
		else if(wlanState == "connected" && wlanIp.empty())
			shownState = "getting ip";
		else if(wlanState == "connected")
			shownState = "connected";

		snprintf(line, sizeof(line), "state: %s", shownState);
		info += line;
		info += '\n';

		/*only a fully connected link shows details; every other state
		  shows just the state line*/
		if(strcmp(shownState, "connected") != 0) {
			infoPanel->setText(info);
			return;
		}

		if(!wlanIp.empty()) {
			snprintf(line, sizeof(line), "ip: %s", wlanIp.c_str());
			info += line;
			info += '\n';
		}

                if(!wlanMac.empty()) {
                        snprintf(line, sizeof(line), "mac: %s", wlanMac.c_str());
                        info += line;
                        info += '\n';
                }

		/*show exactly one entry: the AP we are connected to; every other
		  scan result is ignored*/
		if(wlanSsid.empty()) {
			infoPanel->setText(info);
			return;
		}

		std::string bssid = wlanBssid;
		std::string auth = wlanAuth;
		std::string cipher = wlanCipher;
		int32_t channel = wlanChannel;
		int32_t rssi = wlanRssi;
		for(uint32_t i=0; i<wifiList->getCount(); i++) {
			const WifiItem* it = wifiList->getItem(i);
			if(it != NULL && it->ssid == wlanSsid) {
				bssid = it->bssid;
				auth = it->auth;
				cipher = it->cipher;
				channel = it->channel;
				rssi = it->rssi;
				break;
			}
		}

		info += '\n';
		snprintf(line, sizeof(line), "ssid: %s", wlanSsid.c_str());
		info += line;
		info += '\n';

		if(!bssid.empty()) {
			snprintf(line, sizeof(line), "bssid: %s", bssid.c_str());
			info += line;
			info += '\n';
		}

		snprintf(line, sizeof(line), "channel: %d", channel);
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "rssi: %d", rssi);
		info += line;
		info += '\n';

		if(!auth.empty()) {
			snprintf(line, sizeof(line), "auth: %s", auth.c_str());
			info += line;
			info += '\n';
		}

		if(!cipher.empty()) {
			snprintf(line, sizeof(line), "cipher: %s", cipher.c_str());
			info += line;
			info += '\n';
		}

		infoPanel->setText(info);
	}
};

void InfoPanel::onTimer(uint32_t timerFPS, uint32_t timerSteps) {
	if(app != NULL)
		app->poll(timerFPS, timerSteps);
}

void WifiList::onSelect(int32_t index) {
	if(app != NULL)
		app->onSelected(index);
}

void WifiList::setItems(const WifiItem* src, uint32_t num, const std::string& selectedSsid) {
	count = num;
	if(count > MAX_WIFI_ITEMS)
		count = MAX_WIFI_ITEMS;

	for(uint32_t i=0; i<count; i++)
		items[i] = src[i];

	setItemNum(count);
	if(count == 0) {
		update();
		return;
	}

	int32_t sel = 0;
	for(uint32_t i=0; i<count; i++) {
		if(items[i].ssid == selectedSsid) {
			sel = (int32_t)i;
			break;
		}
	}
	select(sel);
	update();
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	X x;
	WifiWin win;

	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);

	Container* left = new Container();
	left->setType(Container::VERTICAL);
	root->add(left);

	Container* listRow = new Container();
	listRow->setType(Container::HORIZONTAL);
	left->add(listRow);

	WifiList* wifiList = new WifiList(&win);
	wifiList->setItemSize(24);
	listRow->add(wifiList);

	Scroller* scroller = new Scroller();
	scroller->fix(8, 0);
	listRow->add(scroller);
	wifiList->setScrollerV(scroller);

	Container* controls = new Container();
	controls->setType(Container::VERTICAL);
	controls->fix(0, 84);
	left->add(controls);

	Blank* gap = new Blank();
	gap->fix(0, 6);
	controls->add(gap);

	EditLine* password = new EditLine();
	password->fix(0, 36);
	controls->add(password);
	root->focus(password);

	gap = new Blank();
	gap->fix(0, 6);
	controls->add(gap);

	Container* buttonRow = new Container();
	buttonRow->setType(Container::HORIZONTAL);
	buttonRow->fix(0, 36);
	controls->add(buttonRow);

	LabelButton* connect = new LabelButton("connect");
	connect->setEventFunc(WifiWin::connectClick, &win);
	buttonRow->add(connect);

	gap = new Blank();
	gap->fix(6, 0);
	buttonRow->add(gap);

	LabelButton* refresh = new LabelButton("refresh");
	refresh->setEventFunc(WifiWin::refreshClick, &win);
	buttonRow->add(refresh);

	Splitter* splitter = new Splitter();
	splitter->attach(left);
	root->add(splitter);

	InfoPanel* info = new InfoPanel(&win);
	root->add(info);

	win.setWidgets(wifiList, password, info);
	win.open(&x, -1, -1, -1, 560, 320, "xwifi", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(4);
	win.triggerScan();
	win.refreshState();
	win.refreshList();
	win.updateInfo();

	widgetXRun(&x, &win);
	return 0;
}

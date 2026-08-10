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
#include <string>

using namespace Ewok;

static const char* WLAN_DEV = "/dev/wl0";
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

class WifiList: public List {
	WifiWin* app;
	WifiItem items[MAX_WIFI_ITEMS];
	uint32_t count;
	std::string emptyText;

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
		snprintf(line, sizeof(line), "%c %s (%ddBm)",
				item.connected ? '*' : ' ',
				item.ssid.c_str(),
				item.rssi);
		graph_draw_text_font(g, r.x+4, r.y+4, line, theme->getFont(), theme->basic.fontSize, fg);
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
	std::string statusText;
	std::string stateText;
	bool scanReady;
	uint32_t scanWaitTicks;

public:
	WifiWin() {
		wifiList = NULL;
		password = NULL;
		infoPanel = NULL;
		scanReady = false;
		scanWaitTicks = 0;
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
			triggerScan();
			refreshList();
			refreshState();
			return;
		}

		if((timerSteps % timerFPS) == 0) {
			refreshList();
			refreshState();
		}

		if((timerSteps % (timerFPS * 5)) == 0) {
			triggerScan();
		}
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
		if(item == NULL) {
			statusText = "No Wi-Fi selected.";
			updateInfo();
			return;
		}

		char cmd[192];
		snprintf(cmd, sizeof(cmd), "connect %s %s",
				item->ssid.c_str(),
				password->getContent().c_str());
		char* ret = dev_cmd(WLAN_DEV, cmd);
		if(ret != NULL) {
			statusText = ret;
			free(ret);
		}
		else {
			statusText = "connect failed: dev.cmd returned nothing";
		}

		refreshState();
		refreshList();
	}

	void triggerScan() {
		char* ret = dev_cmd(WLAN_DEV, "scan");
		scanReady = false;
		scanWaitTicks = SCAN_WAIT_TICKS;
		updateEmptyHint();
		if(ret != NULL) {
			statusText = ret;
			free(ret);
		}
		else {
			statusText = "scan command sent";
		}
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

		WifiItem loaded[MAX_WIFI_ITEMS];
		uint32_t num = json_var_array_size(arr);
		uint32_t loadedNum = 0;

		std::string selectedSsid;
		const WifiItem* curr = wifiList->getItem(wifiList->getSelected());
		if(curr != NULL)
			selectedSsid = curr->ssid;

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

			int32_t existed = -1;
			for(uint32_t j=0; j<loadedNum; j++) {
				if(loaded[j].ssid == wifi.ssid) {
					existed = (int32_t)j;
					break;
				}
			}

			if(existed >= 0) {
				WifiItem& old = loaded[existed];
				if(wifi.connected && selectedSsid.empty())
					selectedSsid = wifi.ssid;

				if((wifi.rssi > old.rssi) || (wifi.connected && !old.connected))
					old = wifi;
			}
			else if(loadedNum < MAX_WIFI_ITEMS) {
				loaded[loadedNum] = wifi;
				if(selectedSsid.empty() && loaded[loadedNum].connected)
					selectedSsid = loaded[loadedNum].ssid;
				loadedNum++;
			}
		}

		wifiList->setItems(loaded, loadedNum, selectedSsid);
		updateEmptyHint();
		json_var_unref(arr);
	}

	void refreshState() {
		char* ret = dev_cmd(WLAN_DEV, "state");
		if(ret == NULL)
			return;

		json_var_t* obj = json_parse_str(ret);
		free(ret);
		if(obj == NULL)
			return;

		char line[192];
		std::string info;

		snprintf(line, sizeof(line), "state: %s", json_get_str_def(obj, "state", "unknown"));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "ssid: %s", json_get_str_def(obj, "ssid", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "ip: %s", json_get_str_def(obj, "ip", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "mac: %s", json_get_str_def(obj, "mac", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "bssid: %s", json_get_str_def(obj, "bssid", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "channel: %d", json_get_int_def(obj, "channel", 0));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "rssi: %d", json_get_int_def(obj, "rssi", 0));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "auth: %s", json_get_str_def(obj, "auth", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "cipher: %s", json_get_str_def(obj, "cipher", ""));
		info += line;
		info += '\n';

		snprintf(line, sizeof(line), "reason: %s", json_get_str_def(obj, "reason", ""));
		info += line;

		scanReady = json_get_bool_def(obj, "scan_ready", false);
		if(scanReady)
			scanWaitTicks = 0;
		updateEmptyHint();

		stateText = info;
		json_var_unref(obj);
		updateInfo();
	}

	void updateInfo() {
		std::string info = "WLAN Info\n";
		info += "----------------\n";
		info += stateText;

		const WifiItem* item = wifiList->getItem(wifiList->getSelected());
		if(item != NULL) {
			char line[192];
			info += "\n\nSelected AP\n";
			info += "----------------\n";

			snprintf(line, sizeof(line), "ssid: %s", item->ssid.c_str());
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "bssid: %s", item->bssid.c_str());
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "auth: %s", item->auth.c_str());
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "cipher: %s", item->cipher.c_str());
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "type: %s", item->type.c_str());
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "channel: %d", item->channel);
			info += line;
			info += '\n';

			snprintf(line, sizeof(line), "rssi: %d", item->rssi);
			info += line;
		}

		if(!statusText.empty()) {
			info += "\n\nCommand\n";
			info += "----------------\n";
			info += statusText;
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

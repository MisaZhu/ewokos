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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>

using namespace Ewok;

/* btd (machines/raspix/system/drivers/btd) exposes the classic-BT HCI adapter
   on /dev/bt0. Unlike wland (which answers in json) btd's dev.cmd replies with
   plain "key=value" text lines, so this app parses text instead of json:
     state   -> "state powered=1 ready=1 scanning=0 devices=3 pending=0"
     devices -> "0: device AA:BB:.. class=0x5A020C rssi=-60 connected=1
                 paired=0 name=My Headset\n...\ndevices_done count=N"
   Commands sent: open/close, scan, stop, connect/pair/disconnect <bdaddr>. */
static const char* BT_DEV = "/dev/bt0";

/* must be >= btd's MAX_BT_DEVICES so a full cache is never truncated */
static const uint32_t MAX_BT_ITEMS = 32;
/* ticks to keep showing "searching..." after kicking an inquiry (~5s at 4fps) */
static const uint32_t SCAN_WAIT_TICKS = 20;

struct BtItem {
	std::string addr;
	std::string name;
	uint32_t cod;      /* class of device */
	int32_t  rssi;     /* dBm; btd uses 127/0 for "unknown" */
	bool connected;
	bool paired;

	BtItem(): cod(0), rssi(0), connected(false), paired(false) {
	}
};

class BtWin;

/* ---------------- text helpers (btd answers plain text, not json) --------- */

static bool parseIntField(const char* line, const char* key, int32_t* out) {
	const char* p = strstr(line, key);
	if(p == NULL)
		return false;
	*out = atoi(p + strlen(key));
	return true;
}

static bool parseHexField(const char* line, const char* key, uint32_t* out) {
	const char* p = strstr(line, key);
	if(p == NULL)
		return false;
	*out = (uint32_t)strtoul(p + strlen(key), NULL, 16);
	return true;
}

/* name= is the last field on a device line, so it may contain spaces: take
   everything up to end-of-line and trim trailing blanks. "-" means unknown. */
static void parseNameField(const char* line, const char* key, char* out, size_t outsz) {
	const char* p;
	size_t i = 0;

	if(outsz == 0)
		return;
	out[0] = 0;
	p = strstr(line, key);
	if(p == NULL)
		return;
	p += strlen(key);
	while(*p != 0 && *p != '\n' && *p != '\r' && i + 1 < outsz)
		out[i++] = *p++;
	out[i] = 0;
	while(i > 0 && (out[i-1] == ' ' || out[i-1] == '\t'))
		out[--i] = 0;
}

/* pull the "AA:BB:CC:DD:EE:FF" token that follows the "device " prefix */
static bool parseAddrField(const char* line, char* out, size_t outsz) {
	const char* p = strstr(line, "device ");
	if(p == NULL || outsz < 18)
		return false;
	p += strlen("device ");
	if(strlen(p) < 17)
		return false;
	memcpy(out, p, 17);
	out[17] = 0;
	return out[2] == ':' && out[5] == ':' && out[8] == ':' &&
			out[11] == ':' && out[14] == ':';
}

/* split the multi-line "devices" reply and decode each device row */
static uint32_t parseDeviceList(const char* text, BtItem* items, uint32_t max) {
	uint32_t n = 0;
	const char* p = text;
	char line[256];

	while(*p != 0 && n < max) {
		const char* e = strchr(p, '\n');
		size_t len = (e == NULL) ? strlen(p) : (size_t)(e - p);
		if(len >= sizeof(line))
			len = sizeof(line) - 1;
		memcpy(line, p, len);
		line[len] = 0;

		char addr[24];
		if(parseAddrField(line, addr, sizeof(addr))) {
			BtItem it;
			char name[80];
			int32_t v = 0;
			uint32_t hv = 0;

			it.addr = addr;
			if(parseHexField(line, "class=0x", &hv))
				it.cod = hv;
			if(parseIntField(line, "rssi=", &v))
				it.rssi = v;
			if(parseIntField(line, "connected=", &v))
				it.connected = (v != 0);
			if(parseIntField(line, "paired=", &v))
				it.paired = (v != 0);
			parseNameField(line, "name=", name, sizeof(name));
			if(strcmp(name, "-") != 0)
				it.name = name;
			items[n++] = it;
		}

		if(e == NULL)
			break;
		p = e + 1;
	}
	return n;
}

/* map a negative dBm reading to 0..4 signal bars; btd reports 0/127 as unknown */
static int32_t rssiLevel(int32_t rssi) {
	if(rssi >= 0)
		return 0;
	if(rssi >= -50)
		return 4;
	if(rssi >= -60)
		return 3;
	if(rssi >= -70)
		return 2;
	return 1;
}

/* Bluetooth "major device class" is bits 8..12 of the class-of-device */
static const char* codTypeName(uint32_t cod) {
	switch((cod >> 8) & 0x1f) {
	case 0x01: return "Computer";
	case 0x02: return "Phone";
	case 0x03: return "Network";
	case 0x04: return "Audio/Video";
	case 0x05: return "Peripheral";
	case 0x06: return "Imaging";
	case 0x07: return "Wearable";
	case 0x08: return "Toy";
	case 0x09: return "Health";
	default:   return "Unknown";
	}
}

/* ---------------- info panel (multi-line text, same as xwifi) ------------- */

class InfoPanel: public Widget {
	BtWin* app;
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
	InfoPanel(BtWin* app): app(app) {
	}

	void setText(const std::string& s) {
		if(text == s)
			return;
		text = s;
		update();
	}
};

/* ---------------- device list -------------------------------------------- */

class BtList: public List {
	BtWin* app;
	BtItem items[MAX_BT_ITEMS];
	uint32_t count;
	std::string emptyText;

	/* classic stepped signal bars pinned to the row's right edge so every
	   row's icon lines up regardless of the name/addr length */
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

		const BtItem& item = items[index];
		uint32_t fg = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			fg = theme->basic.selectColor;
		}

		/* two status glyphs: '*' connected, 'K' paired (link key stored) */
		char status[3];
		status[0] = item.connected ? '*' : ' ';
		status[1] = item.paired ? 'K' : ' ';
		status[2] = 0;

		const char* shown = item.name.empty() ? item.addr.c_str() : item.name.c_str();
		char line[192];
		snprintf(line, sizeof(line), "%s %s", status, shown);
		graph_draw_text_font(g, r.x+4, r.y+4, line, theme->getFont(), theme->basic.fontSize, fg);
		drawSignalIcon(g, theme, r, rssiLevel(item.rssi), fg);
	}

	void onSelect(int32_t index);

public:
	BtList(BtWin* app): app(app), count(0) {
		emptyText = "Searching devices...";
	}

	void setItems(const BtItem* src, uint32_t num, const std::string& selectedAddr);
	void setEmptyText(const std::string& s) {
		emptyText = s;
		update();
	}
	const BtItem* getItem(int32_t index) const {
		if(index < 0 || (uint32_t)index >= count)
			return NULL;
		return &items[index];
	}
	uint32_t getCount(void) const {
		return count;
	}
};

/* ---------------- main window -------------------------------------------- */

class BtWin: public WidgetWin {
	BtList* btList;
	EditLine* pinEdit;
	InfoPanel* infoPanel;
	LabelButton* powerBtn;

	bool adapterPresent;
	bool powered;
	bool ready;
	bool scanning;
	uint32_t scanWaitTicks;

public:
	BtWin() {
		btList = NULL;
		pinEdit = NULL;
		infoPanel = NULL;
		powerBtn = NULL;
		adapterPresent = false;
		powered = false;
		ready = false;
		scanning = false;
		scanWaitTicks = 0;
	}

	static void click(Widget* wd, xevent_t* evt, void* arg, void (BtWin::*fn)()) {
		(void)wd;
		if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
			return;
		(((BtWin*)arg)->*fn)();
	}
	static void connectClick(Widget* wd, xevent_t* evt, void* arg) { click(wd, evt, arg, &BtWin::connectSelected); }
	static void pairClick(Widget* wd, xevent_t* evt, void* arg)    { click(wd, evt, arg, &BtWin::pairSelected); }
	static void disconnectClick(Widget* wd, xevent_t* evt, void* arg) { click(wd, evt, arg, &BtWin::disconnectSelected); }
	static void scanClick(Widget* wd, xevent_t* evt, void* arg)    { click(wd, evt, arg, &BtWin::manualScan); }
	static void powerClick(Widget* wd, xevent_t* evt, void* arg)   { click(wd, evt, arg, &BtWin::togglePower); }

	void setWidgets(BtList* list, EditLine* pin, InfoPanel* info, LabelButton* power) {
		btList = list;
		pinEdit = pin;
		infoPanel = info;
		powerBtn = power;
		updateEmptyHint();
	}

	const BtItem* selectedItem() {
		if(btList == NULL)
			return NULL;
		return btList->getItem(btList->getSelected());
	}

	void updateEmptyHint() {
		if(btList == NULL || btList->getCount() > 0)
			return;
		if(!adapterPresent)
			btList->setEmptyText("No Bluetooth adapter (/dev/bt0).");
		else if(!powered)
			btList->setEmptyText("Bluetooth is off. Click 'power on'.");
		else if(scanning || scanWaitTicks > 0)
			btList->setEmptyText("Searching devices...");
		else
			btList->setEmptyText("No devices found. Click scan.");
	}

	void poll(uint32_t timerFPS, uint32_t timerSteps) {
		if(scanWaitTicks > 0) {
			scanWaitTicks--;
			if(scanWaitTicks == 0)
				updateEmptyHint();
		}

		if(timerSteps == 1) {
			refreshState();
			refreshList();
			if(powered)
				triggerScan();
			updateInfo();
			return;
		}

		/* state is a cheap cached read in btd: poll it fast so power/scan
		   transitions and connect->connected show up promptly; the device
		   list refreshes once a second while an inquiry streams results in */
		if((timerSteps % 2) == 0)
			refreshState();
		if(timerFPS > 0 && (timerSteps % timerFPS) == 0)
			refreshList();
	}

	void onSelected(int32_t index) {
		(void)index;
		updateInfo();
	}

	/* fire-and-forget a btd command, discarding the reply text */
	void sendCmd(const char* cmd) {
		char* ret = dev_cmd(BT_DEV, cmd);
		if(ret != NULL)
			free(ret);
	}

	void triggerScan() {
		if(!powered)
			return;
		sendCmd("scan 10");
		scanning = true;
		scanWaitTicks = SCAN_WAIT_TICKS;
		updateEmptyHint();
	}

	void manualScan() {
		triggerScan();
		refreshList();
		refreshState();
		updateInfo();
	}

	void connectSelected() {
		const BtItem* item = selectedItem();
		if(item == NULL)
			return;
		char cmd[128];
		sendCmd("stop");   /* cancel any inquiry before creating a connection */
		snprintf(cmd, sizeof(cmd), "connect %s", item->addr.c_str());
		sendCmd(cmd);
		refreshState();
		refreshList();
	}

	void pairSelected() {
		const BtItem* item = selectedItem();
		if(item == NULL)
			return;
		char cmd[160];
		std::string pin = (pinEdit != NULL) ? pinEdit->getContent() : std::string("");
		sendCmd("stop");
		if(pin.empty())
			snprintf(cmd, sizeof(cmd), "pair %s", item->addr.c_str());
		else
			snprintf(cmd, sizeof(cmd), "pair %s %s", item->addr.c_str(), pin.c_str());
		sendCmd(cmd);
		refreshState();
		refreshList();
	}

	void disconnectSelected() {
		const BtItem* item = selectedItem();
		if(item == NULL)
			return;
		char cmd[128];
		snprintf(cmd, sizeof(cmd), "disconnect %s", item->addr.c_str());
		sendCmd(cmd);
		refreshState();
		refreshList();
	}

	void togglePower() {
		sendCmd(powered ? "close" : "open");
		refreshState();
		if(powered) {
			refreshList();
			triggerScan();
		}
		else {
			refreshList();
		}
		updateInfo();
	}

	void refreshState() {
		char* ret = dev_cmd(BT_DEV, "state");
		int32_t v = 0;

		if(ret == NULL) {
			/* btd is not running / node missing */
			adapterPresent = false;
			powered = false;
			ready = false;
			scanning = false;
			updateEmptyHint();
			updateInfo();
			return;
		}
		adapterPresent = true;
		if(parseIntField(ret, "powered=", &v))
			powered = (v != 0);
		if(parseIntField(ret, "ready=", &v))
			ready = (v != 0);
		if(parseIntField(ret, "scanning=", &v))
			scanning = (v != 0);
		free(ret);

		if(powerBtn != NULL)
			powerBtn->setLabel(powered ? "power off" : "power on");
		updateEmptyHint();
		updateInfo();
	}

	void refreshList() {
		if(btList == NULL)
			return;

		char* ret = dev_cmd(BT_DEV, "devices");
		if(ret == NULL)
			return;

		std::string selAddr;
		const BtItem* curr = selectedItem();
		if(curr != NULL)
			selAddr = curr->addr;

		BtItem parsed[MAX_BT_ITEMS];
		uint32_t n = parseDeviceList(ret, parsed, MAX_BT_ITEMS);
		free(ret);

		btList->setItems(parsed, n, selAddr);
		updateEmptyHint();
		updateInfo();
	}

	void updateInfo() {
		if(infoPanel == NULL)
			return;

		char line[192];
		std::string info = "Bluetooth\n";
		info += "----------------\n";

		if(!adapterPresent) {
			info += "adapter: not found\n";
			info += "(/dev/bt0 missing)";
			infoPanel->setText(info);
			return;
		}

		snprintf(line, sizeof(line), "adapter: %s", powered ? "on" : "off");
		info += line;
		info += '\n';

		if(powered) {
			const char* st = scanning ? "scanning" : (ready ? "ready" : "init");
			snprintf(line, sizeof(line), "status: %s", st);
			info += line;
			info += '\n';
		}

		const BtItem* it = selectedItem();
		if(it == NULL) {
			info += "\nselect a device";
			infoPanel->setText(info);
			return;
		}

		info += '\n';
		snprintf(line, sizeof(line), "name: %s", it->name.empty() ? "-" : it->name.c_str());
		info += line;
		info += '\n';
		snprintf(line, sizeof(line), "addr: %s", it->addr.c_str());
		info += line;
		info += '\n';
		snprintf(line, sizeof(line), "type: %s", codTypeName(it->cod));
		info += line;
		info += '\n';
		if(it->rssi < 0)
			snprintf(line, sizeof(line), "rssi: %d dBm", it->rssi);
		else
			snprintf(line, sizeof(line), "rssi: -");
		info += line;
		info += '\n';
		snprintf(line, sizeof(line), "paired: %s", it->paired ? "yes" : "no");
		info += line;
		info += '\n';
		snprintf(line, sizeof(line), "connected: %s", it->connected ? "yes" : "no");
		info += line;
		info += '\n';

		infoPanel->setText(info);
	}
};

void InfoPanel::onTimer(uint32_t timerFPS, uint32_t timerSteps) {
	if(app != NULL)
		app->poll(timerFPS, timerSteps);
}

void BtList::onSelect(int32_t index) {
	if(app != NULL)
		app->onSelected(index);
}

void BtList::setItems(const BtItem* src, uint32_t num, const std::string& selectedAddr) {
	count = num;
	if(count > MAX_BT_ITEMS)
		count = MAX_BT_ITEMS;

	for(uint32_t i=0; i<count; i++)
		items[i] = src[i];

	setItemNum(count);
	if(count == 0) {
		update();
		return;
	}

	int32_t sel = 0;
	for(uint32_t i=0; i<count; i++) {
		if(items[i].addr == selectedAddr) {
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
	BtWin win;

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

	BtList* btList = new BtList(&win);
	btList->setItemSize(24);
	listRow->add(btList);

	Scroller* scroller = new Scroller();
	scroller->fix(8, 0);
	listRow->add(scroller);
	btList->setScrollerV(scroller);

	Container* controls = new Container();
	controls->setType(Container::VERTICAL);
	controls->fix(0, 122);
	left->add(controls);

	Blank* gap = new Blank();
	gap->fix(0, 6);
	controls->add(gap);

	/* pairing PIN / passkey; btd defaults to "0000" when left blank */
	EditLine* pin = new EditLine();
	pin->fix(0, 30);
	controls->add(pin);
	root->focus(pin);

	gap = new Blank();
	gap->fix(0, 6);
	controls->add(gap);

	Container* row1 = new Container();
	row1->setType(Container::HORIZONTAL);
	row1->fix(0, 34);
	controls->add(row1);

	LabelButton* connectBtn = new LabelButton("connect");
	connectBtn->setEventFunc(BtWin::connectClick, &win);
	row1->add(connectBtn);

	gap = new Blank();
	gap->fix(6, 0);
	row1->add(gap);

	LabelButton* pairBtn = new LabelButton("pair");
	pairBtn->setEventFunc(BtWin::pairClick, &win);
	row1->add(pairBtn);

	gap = new Blank();
	gap->fix(6, 0);
	row1->add(gap);

	LabelButton* discBtn = new LabelButton("disconnect");
	discBtn->setEventFunc(BtWin::disconnectClick, &win);
	row1->add(discBtn);

	gap = new Blank();
	gap->fix(0, 6);
	controls->add(gap);

	Container* row2 = new Container();
	row2->setType(Container::HORIZONTAL);
	row2->fix(0, 34);
	controls->add(row2);

	LabelButton* scanBtn = new LabelButton("scan");
	scanBtn->setEventFunc(BtWin::scanClick, &win);
	row2->add(scanBtn);

	gap = new Blank();
	gap->fix(6, 0);
	row2->add(gap);

	LabelButton* powerBtn = new LabelButton("power off");
	powerBtn->setEventFunc(BtWin::powerClick, &win);
	row2->add(powerBtn);

	Splitter* splitter = new Splitter();
	splitter->attach(left);
	root->add(splitter);

	InfoPanel* info = new InfoPanel(&win);
	root->add(info);

	win.setWidgets(btList, pin, info, powerBtn);
	win.open(&x, -1, -1, -1, 600, 360, "xbt", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(4);
	win.refreshState();
	win.refreshList();
	win.updateInfo();

	widgetXRun(&x, &win);
	return 0;
}

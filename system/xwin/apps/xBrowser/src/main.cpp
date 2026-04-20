// Main entry point using widget++

#include <x++/X.h>
#include <Widget/WidgetWin.h>
#include <Widget/RootWidget.h>
#include <Widget/WidgetX.h>
#include <Widget/EditLine.h>
#include <Widget/Scroller.h>
#include <Widget/Container.h>
#include <ewoksys/keydef.h>


#include "WidgetWebview/WidgetWebview.h"

using namespace Ewok;

static void onInputFunc(Widget* wd, uint32_t key, void* arg) {
    WidgetWebview* webview = (WidgetWebview*)arg;
    EditLine* editline = (EditLine*)wd;
    if(webview == NULL || editline->getContent().empty())
        return;
    if(key != KEY_ENTER)
        return;
    std::string full_url = editline->getContent();
    if(full_url.find("://") == std::string::npos) {
        full_url = "https://" + editline->getContent();
        editline->setContent(full_url);
    }

    webview->loadHtml(full_url);
}

int main(int argc, char* argv[])
{
    X x;
    WidgetWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::VERTICAL);

    EditLine* editline = new EditLine();
    root->add(editline);
    editline->fix(0, 24);
    root->focus(editline);

    // Create a horizontal container for webview and vertical scroller
    Container* c = new Container();
    c->setType(Container::HORIZONTAL);
    root->add(c);

    WidgetWebview* webview = new WidgetWebview();
    c->add(webview);

    editline->setOnInputFunc(onInputFunc, webview);

    // Add vertical scroller
    Scroller* sr = new Scroller();
    sr->fix(8, 0);
    webview->setScrollerV(sr);
    c->add(sr);

    editline->setOnInputFunc(onInputFunc, webview);

    win.open(&x, -1, -1, -1, 0, 0, "HTML Browser", XWIN_STYLE_NORMAL);

    webview->loadCSS("res://html/default.css");
    if(argc > 1) {
        editline->disable();
        editline->setContent(argv[1]);
        webview->loadHtml(argv[1]);
    }
    else {
        editline->enable();
        webview->loadHtml("res://html/default.html");
    }

    widgetXRun(&x, &win);
    return 0;
}

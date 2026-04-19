// Webview widget implementation

#include "WidgetWebview/WidgetWebview.h"
#include "XContainer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <graph/graph.h>
#include <x++/X.h>

using namespace Ewok;

WidgetWebview::WidgetWebview()
    : m_clientWidth(640)
    , m_clientHeight(480)
    , m_doc(nullptr)
{
    m_container = std::make_shared<XContainer>(&m_browser_context, this);
}

WidgetWebview::~WidgetWebview()
{
    delete m_doc;
    m_doc = nullptr;
    // Note: m_container is std::shared_ptr, no need to delete manually
    m_container.reset();
}

bool WidgetWebview::init()
{
    return true;
}

bool WidgetWebview::loadHtml(const std::string& url)
{
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    return loadHtmlContent(strContents);
}

bool WidgetWebview::loadCSS(const std::string& url)
{
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    return loadCSSContent(strContents);
}

bool WidgetWebview::loadCSSContent(const std::string& content)
{
    if (!content.empty()) {
        m_browser_context.load_master_stylesheet(content.c_str());
    }
    return true;
}

bool WidgetWebview::loadHtmlContent(const std::string& content)
{
    if (!content.empty()) {
        m_doc = litehtml::document::createFromString(content.c_str(), m_container.get(), &m_browser_context);
        if (m_doc) {
            m_doc->render(m_clientWidth);
        }
    }
    return m_doc != nullptr;
}

void WidgetWebview::onRepaint(graph_t* g, XTheme* theme, const grect_t& r)
{
    (void)theme;
    (void)r;

    if (g == NULL)
        return;

    m_container->setGraph(g);

    // Clear background to white
    graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);

    litehtml::position pos(r.x, r.y, r.w, r.h);

    if (m_doc) {
        m_doc->draw((litehtml::uint_ptr)g, r.x, r.y, &pos);
    }
}

void WidgetWebview::onResize()
{
    m_clientWidth = area.w;
    m_clientHeight = area.h;
    m_container->set_client_size(m_clientWidth, m_clientHeight);

    if (m_doc) {
        m_doc->render(m_clientWidth);
    }
}

void WidgetWebview::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "url") {
		const char* url = json_var_get_str(value);
		loadHtml(url);
	}	
    else if(attr == "css") {
		const char* css = json_var_get_str(value);
		loadCSS(css);
	}
}

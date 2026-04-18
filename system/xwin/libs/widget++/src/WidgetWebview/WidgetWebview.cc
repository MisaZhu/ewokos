// Webview widget implementation

#include "WidgetWebview/WidgetWebview.h"
#include "XContainer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <graph/graph.h>

using namespace Ewok;

static std::string readFileContents(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::string contents;
    char c;
    while (file.get(c)) {
        contents += c;
    }
    return contents;
}

WidgetWebview::WidgetWebview()
    : m_clientWidth(640)
    , m_clientHeight(480)
    , m_doc(nullptr)
{
    m_container = std::make_shared<XContainer>(&m_browser_context, this);
}

WidgetWebview::~WidgetWebview()
{
    m_doc.reset();
    m_container.reset();
}

bool WidgetWebview::init()
{
    return true;
}

bool WidgetWebview::load(const std::string& url)
{
    return loadHtmlContent(url);
}

bool WidgetWebview::loadHtmlContent(const std::string& url)
{
    {
        std::string strContents = readFileContents("/data/html/master.css");
        if (!strContents.empty()) {
            m_browser_context.load_master_stylesheet(strContents.c_str());
        }
    }

    if(url.starts_with("/"))
    {
        std::string strContents = readFileContents(url.c_str());
        if (!strContents.empty()) {
            m_doc = litehtml::document::createFromString(strContents.c_str(), m_container.get(), &m_browser_context);
            if (m_doc) {
                m_doc->render(m_clientWidth);
            }
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
    graph_fill_rect(g, area.x, area.y, area.w, area.h, 0xFFFFFFFF);

    litehtml::position pos(area.x, area.y, area.w, area.h);

    if (m_doc) {
        m_doc->draw((litehtml::uint_ptr)g, area.x, area.y, &pos);
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

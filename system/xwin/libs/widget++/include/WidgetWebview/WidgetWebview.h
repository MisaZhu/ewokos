// Webview widget for litehtml rendering

#pragma once

#include <Widget/Widget.h>
#include <litehtml.h>
#include <memory>

class XContainer;

namespace Ewok {

class WidgetWebview : public Widget {
public:
    WidgetWebview();
    virtual ~WidgetWebview();

    bool load(const std::string& url);

protected:
    virtual void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) override;
    virtual void onResize() override;

private:
    bool init();
    bool loadHtmlContent(const std::string& url);

    std::shared_ptr<XContainer> m_container;
    litehtml::document::ptr       m_doc;
    litehtml::context            m_browser_context;

    int m_clientWidth;
    int m_clientHeight;
};

}

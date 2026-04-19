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

    bool loadHtml(const std::string& url);
    bool loadCSS(const std::string& url);

    bool loadCSSContent(const std::string& content);
    bool loadHtmlContent(const std::string& content);

protected:
    virtual void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) override;
    virtual void onResize() override;
    virtual void setAttr(const string& attr, json_var_t*value) override;
    virtual const std::string loadURL(const std::string& url);
private:
    bool init();

    std::shared_ptr<XContainer> m_container;
    litehtml::document::ptr       m_doc;
    litehtml::context            m_browser_context;

    int m_clientWidth;
    int m_clientHeight;
};

}

// Webview widget for litehtml rendering

#pragma once

#include <Widget/Scrollable.h>
#include <litehtml.h>
#include <memory>
#include <queue>
#include <pthread.h>

class XContainer;

struct HttpTask {
    static const int TASK_HTML   = 0;
    static const int TASK_CSS    = 1;
    static const int TASK_IMAGE  = 2;
    std::string url;
    int type;
};

// Forward declaration in global namespace
extern "C" void* _task_thread(void* p);

namespace Ewok {

class WidgetWebview : public Scrollable {
public:
    WidgetWebview();
    virtual ~WidgetWebview();

    bool addTask(const HttpTask& task);
    bool getTask(HttpTask& task);

    bool loadHtml(const std::string& url);
    void setDefaultCSS(const std::string& url);
    bool loadCSS(const std::string& url);
    bool loadImage(const std::string& url);

    bool loadCSSContent(const std::string& content);
    bool loadHtmlContent(const std::string& content);
    bool loadImageContent(const std::string& url, uint8_t* data, int sz);

    friend void* ::_task_thread(void* p);
protected:
    virtual void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) override;
    virtual void onResize() override;
    virtual void setAttr(const string& attr, json_var_t*value) override;

    virtual bool onScroll(int step, bool horizontal) override;
    virtual void updateScroller() override;
    virtual bool onMouse(xevent_t* ev) override;

    bool loadHtmlTask(const std::string& url);
    bool loadCSSTask(const std::string& url);
    bool loadImageTask(const std::string& url);

    virtual void onTaskStart(const HttpTask& task) {}
    virtual void onTaskEnd(const HttpTask& task) {}
    virtual void onTaskFailed(const HttpTask& task) {}
    virtual void onTasksEnd() {}
private:
    std::string m_defaultCSSUrl;
    XContainer*                   m_container;
    litehtml::document::ptr       m_doc;
    litehtml::context            m_browser_context;

    bool m_task_running;
    bool m_task_ended;
    int m_clientWidth;
    int m_clientHeight;

    // Task queue
    std::queue<HttpTask> m_taskQueue;
    pthread_mutex_t m_taskMutex;

    // Mutex for protecting shared resources (m_doc, m_browser_context)
    pthread_mutex_t m_renderMutex;

    // Scroll offsets
    int m_scrollX;
    int m_scrollY;
};

}

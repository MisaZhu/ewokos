// Webview widget implementation

#include "WidgetWebview/WidgetWebview.h"
#include "XContainer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <graph/graph.h>
#include <x++/X.h>
#include <ewoksys/proc.h>
#include <deque>

using namespace Ewok;

// Explicit template instantiation for deque<HttpTask>
template class std::deque<HttpTask>;

WidgetWebview::WidgetWebview()
    : m_clientWidth(640)
    , m_clientHeight(480)
    , m_doc(nullptr)
    , m_scrollX(0)
    , m_scrollY(0)
{
    m_container = std::make_shared<XContainer>(&m_browser_context, this);
    m_task_running = false;
    m_task_ended = false;
    pthread_mutex_init(&m_taskMutex, NULL);
    pthread_mutex_init(&m_renderMutex, NULL);
}

WidgetWebview::~WidgetWebview()
{
    // Signal task thread to end
    m_task_ended = true;

    // Wait for task thread to finish
    while(m_task_running) {
        proc_usleep(10000);
    }

    pthread_mutex_lock(&m_renderMutex);
    delete m_doc;
    m_doc = nullptr;
    // Note: m_container is std::shared_ptr, no need to delete manually
    m_container.reset();
    pthread_mutex_unlock(&m_renderMutex);

    pthread_mutex_destroy(&m_taskMutex);
    pthread_mutex_destroy(&m_renderMutex);
}

bool WidgetWebview::init()
{
    return true;
}

void* _task_thread(void* p)
{
    WidgetWebview* widget = (WidgetWebview*)p;
    HttpTask task;
    
    while(!widget->m_task_ended) {
        if (widget->getTask(task)) {
            // Process task
            if (task.type == HttpTask::TASK_HTML) {
                klog("loadHtmlTask: %s\n", task.url.c_str());
                widget->loadHtmlTask(task.url);
                klog("loadHtmlTask: %s done\n", task.url.c_str());
            } else if (task.type == HttpTask::TASK_CSS) {
                widget->loadCSSTask(task.url);
            } else if (task.type == HttpTask::TASK_IMAGE) {
                widget->loadImageTask(task.url);
            }
        }
        else {
            // No task, sleep a bit
            proc_usleep(10000);
        }
    }
    
    widget->m_task_running = false;
    return nullptr;
}

bool WidgetWebview::addTask(const HttpTask& task)
{
    klog("task: %s\n", task.url.c_str());
    pthread_mutex_lock(&m_taskMutex);
    m_taskQueue.push(task);
    pthread_mutex_unlock(&m_taskMutex);

    if (!m_task_running) {
        m_task_running = true;
        pthread_t tid;
        pthread_create(&tid, NULL, _task_thread, this);
    }
    return true;
}

bool WidgetWebview::getTask(HttpTask& task)
{
    pthread_mutex_lock(&m_taskMutex);

    // Check if we should exit
    if (m_task_ended && m_taskQueue.empty()) {
        pthread_mutex_unlock(&m_taskMutex);
        return false;
    }

    // Get task from queue
    if (!m_taskQueue.empty()) {
        task = m_taskQueue.front();
        m_taskQueue.pop();
        pthread_mutex_unlock(&m_taskMutex);
        return true;
    }

    pthread_mutex_unlock(&m_taskMutex);
    return false;
}

bool WidgetWebview::loadHtml(const std::string& url)
{
    addTask({url, HttpTask::TASK_HTML});
    return true;
}

bool WidgetWebview::loadCSS(const std::string& url)
{
    addTask({url, HttpTask::TASK_CSS});
    return true;
}

bool WidgetWebview::loadHtmlTask(const std::string& url)
{
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    return loadHtmlContent(strContents);
}

bool WidgetWebview::loadCSSTask(const std::string& url)
{
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    return loadCSSContent(strContents);
}

bool WidgetWebview::loadImageTask(const std::string& url)
{
    int sz;
    uint8_t* content = XContainer::loadURL(url, &sz);
    if(content == NULL) {
        return false;
    }
    bool res = loadImageContent(url, content, sz);
    free(content);
    return res;
}

bool WidgetWebview::loadCSSContent(const std::string& content)
{
    pthread_mutex_lock(&m_renderMutex);
    if (!content.empty()) {
        m_browser_context.load_master_stylesheet(content.c_str());
    }
    pthread_mutex_unlock(&m_renderMutex);
    return true;
}

bool WidgetWebview::loadImageContent(const std::string& url, uint8_t* content, int sz)
{
    bool res = false;
    pthread_mutex_lock(&m_renderMutex);
    if (content != NULL && sz > 0) {
        res = m_container->loadImageData(url, content, sz);
        if (res && m_doc) {
            m_doc->render(m_clientWidth);
        }
    }
    pthread_mutex_unlock(&m_renderMutex);
    if(res)
        update();
    return res;
}

bool WidgetWebview::loadHtmlContent(const std::string& content)
{
    pthread_mutex_lock(&m_renderMutex);
    //sout(content.c_str(), content.size());
    if (!content.empty()) {
        m_doc = litehtml::document::createFromString(content.c_str(), m_container.get(), &m_browser_context);
        if (m_doc) {
            m_doc->render(m_clientWidth);
        }
    }
    pthread_mutex_unlock(&m_renderMutex);
    m_scrollX = 0;
    m_scrollY = 0;
    updateScroller();
    update();
    return m_doc != nullptr;
}

void WidgetWebview::onRepaint(graph_t* g, XTheme* theme, const grect_t& r)
{
    (void)theme;

    if (g == NULL)
        return;

    m_container->setGraph(g);

    // Clear background to white
    graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);

    litehtml::position pos(r.x, r.y, r.w, r.h);

    pthread_mutex_lock(&m_renderMutex);
    if (m_doc) {
        m_doc->draw((litehtml::uint_ptr)g, r.x - m_scrollX, r.y - m_scrollY, &pos);
    }
    pthread_mutex_unlock(&m_renderMutex);
}

void WidgetWebview::onResize()
{
    m_clientWidth = area.w;
    m_clientHeight = area.h;
    m_container->set_client_size(m_clientWidth, m_clientHeight);

    pthread_mutex_lock(&m_renderMutex);
    if (m_doc) {
        m_doc->render(m_clientWidth);
    }
    pthread_mutex_unlock(&m_renderMutex);

    updateScroller();
}

bool WidgetWebview::onScroll(int step, bool horizontal)
{
    pthread_mutex_lock(&m_renderMutex);
    if (!m_doc) {
        pthread_mutex_unlock(&m_renderMutex);
        return false;
    }

    int docWidth = m_doc->width();
    int docHeight = m_doc->height();
    pthread_mutex_unlock(&m_renderMutex);

    if (horizontal) {
        m_scrollX -= step * dragStep;
        if (m_scrollX < 0 || (docWidth - area.w) < 0)
            m_scrollX = 0;
        else if (m_scrollX > (docWidth - area.w))
            m_scrollX = docWidth - area.w;
    } else {
        m_scrollY -= step * dragStep;
        if (m_scrollY < 0 || (docHeight - area.h) < 0)
            m_scrollY = 0;
        else if (m_scrollY > (docHeight - area.h))
            m_scrollY = docHeight - area.h;
    }
    return true;
}

void WidgetWebview::updateScroller()
{
	pthread_mutex_lock(&m_renderMutex);
	if (!m_doc) {
		pthread_mutex_unlock(&m_renderMutex);
		return;
	}

	int docWidth = m_doc->width();
	int docHeight = m_doc->height();
	pthread_mutex_unlock(&m_renderMutex);

	setScrollerInfo(docWidth, m_scrollX, area.w, true);
	setScrollerInfo(docHeight, m_scrollY, area.h, false);
}

bool WidgetWebview::onMouse(xevent_t* ev)
{
	// Call parent class onMouse for drag scrolling
	bool handled = Scrollable::onMouse(ev);
	if (handled)
		return true;

	// Handle mouse wheel scrolling
	if (ev->state == MOUSE_STATE_MOVE) {
		if (ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
			scroll(-1, false);
			return true;
		}
		else if (ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
			scroll(1, false);
			return true;
		}
	}
	return false;
}

void WidgetWebview::setAttr(const string& attr, json_var_t*value) {
	Scrollable::setAttr(attr, value);
	if(attr == "url") {
		const char* url = json_var_get_str(value);
		loadHtml(url);
	}	
    else if(attr == "css") {
		const char* css = json_var_get_str(value);
		loadCSS(css);
	}
}

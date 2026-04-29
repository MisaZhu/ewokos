// Webview widget implementation

#include "WidgetWebview/WidgetWebview.h"
#include "Widget/WidgetWin.h"
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
    , m_container(nullptr)
    , m_scrollX(0)
    , m_scrollY(0)
    , m_needsStyleUpdate(false)
{
    m_container = new XContainer(&m_browser_context, this);
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
    if(m_doc)
        delete m_doc;
    if(m_container)
        delete m_container;
    pthread_mutex_unlock(&m_renderMutex);

    pthread_mutex_destroy(&m_taskMutex);
    pthread_mutex_destroy(&m_renderMutex);
}

void WidgetWebview::setDefaultCSS(const std::string& url)
{
    m_defaultCSSUrl = url;
}

void* _task_thread(void* p)
{
    WidgetWebview* widget = (WidgetWebview*)p;
    HttpTask task;
    
    bool havetask = false;
    while(!widget->m_task_ended) {
        if (widget->getTask(task)) {
            havetask = true;
            bool res = false;
            widget->onTaskStart(task);
            // Process task
            if (task.type == HttpTask::TASK_HTML) {
                widget->getWin()->busy(true);
                res = widget->loadHtmlTask(task.url);
                widget->getWin()->busy(false);
            } else if (task.type == HttpTask::TASK_CSS) {
                res = widget->loadCSSTask(task.url);
            } else if (task.type == HttpTask::TASK_IMAGE) {
                res = widget->loadImageTask(task.url);
            }

            if(res) {
                widget->onTaskEnd(task);
            }
            else {
                widget->onTaskFailed(task);
            }
        }
        else {
            // No task, sleep a bit
            if(havetask) {
                havetask = false;
                widget->onTasksEnd();
            }
            proc_usleep(10000);
        }
    }
    
    widget->m_task_running = false;
    return nullptr;
}

bool WidgetWebview::addTask(const HttpTask& task)
{
    pthread_mutex_lock(&m_taskMutex);

    for (auto& t : m_taskQueue) {
        if (t.url == task.url) {
            pthread_mutex_unlock(&m_taskMutex);
            return false;
        }
    }

    m_taskQueue.push_back(task);
    pthread_mutex_unlock(&m_taskMutex);

    if (!m_task_running) {
        m_task_running = true;
        pthread_t tid;
        pthread_create(&tid, NULL, _task_thread, this);
    }
    return true;
}

void WidgetWebview::removeTask(const std::string& url)
{
    pthread_mutex_lock(&m_taskMutex);
    for (size_t i = 0; i < m_taskQueue.size(); i++) {
        if (m_taskQueue[i].url == url) {
            m_taskQueue.erase(m_taskQueue.begin() + i);
            break;
        }
    }
    pthread_mutex_unlock(&m_taskMutex);
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
    for (size_t i = 0; i < m_taskQueue.size(); i++) {
        if (!m_taskQueue[i].loading) {
            task = m_taskQueue[i];
            m_taskQueue[i].loading = true;
            pthread_mutex_unlock(&m_taskMutex);
            return true;
        }
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
    m_currentHtmlUrl = url;
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        removeTask(url);
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    bool res = loadHtmlContent(strContents);
    removeTask(url);
    return res;
}

bool WidgetWebview::loadCSSTask(const std::string& url)
{
    uint8_t* content = XContainer::loadURL(url, NULL);
    if(content == NULL) {
        removeTask(url);
        return false;
    }
    std::string strContents = (char*)content;
    free(content);
    bool res = loadCSSContent(strContents);
    removeTask(url);
    return res;
}

bool WidgetWebview::loadImageTask(const std::string& url)
{
    int sz;
    uint8_t* content = XContainer::loadURL(url, &sz);
    if(content == NULL) {
        removeTask(url);
        return false;
    }
    bool res = loadImageContent(url, content, sz);
    free(content);
    removeTask(url);
    return res;
}

bool WidgetWebview::loadCSSContent(const std::string& content)
{
    bool res = false;
    pthread_mutex_lock(&m_renderMutex);
    if (!content.empty()) {
        m_browser_context.load_master_stylesheet(content.c_str());
        if (m_doc) {
            // Defer style update to onRepaint to avoid concurrent access to litehtml
            m_needsStyleUpdate = true;
            res = true;
        }
    }
    pthread_mutex_unlock(&m_renderMutex);
    if(res)
        update();  // Trigger repaint which will apply the styles
    return res;
}

bool WidgetWebview::loadImageContent(const std::string& url, uint8_t* content, int sz)
{
    bool res = false;
    pthread_mutex_lock(&m_renderMutex);
    if (content != NULL && sz > 0 && m_container != NULL) {
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
    m_browser_context.master_css().clear(); // Clear CSS styles from browser context
    pthread_mutex_unlock(&m_renderMutex);

    pthread_mutex_lock(&m_taskMutex);
    m_taskQueue.clear();
    pthread_mutex_unlock(&m_taskMutex);
    
    if(!m_defaultCSSUrl.empty()) {
        loadCSSTask(m_defaultCSSUrl);
    }

    pthread_mutex_lock(&m_renderMutex);
    //kout(content.c_str(), content.size());
    if (!content.empty()) {
        // Clean up old document first (before container)
        // This is important because m_doc holds a reference to m_container
        if(m_doc) {
            delete m_doc;
            m_doc = nullptr;
        }

        // Clean up and recreate container (clears images, fonts, inputs)
        // Must be done after m_doc is deleted to avoid use-after-free
        if(m_container)
            delete m_container;
        m_container = new XContainer(&m_browser_context, this);

        m_doc = litehtml::document::createFromString(content.c_str(), m_container, &m_browser_context);
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

    // Clear background to white
    graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);

    litehtml::position pos(r.x, r.y, r.w, r.h);

    pthread_mutex_lock(&m_renderMutex);
    
    // Apply pending style update if any (must be done in main thread)
    if (m_needsStyleUpdate && m_doc) {
        m_doc->update_master_styles();
        m_doc->render(m_clientWidth);
        m_needsStyleUpdate = false;
    }
    
    if (m_container) {
        m_container->setGraph(g);
    }
    if (m_doc) {
        m_doc->draw((litehtml::uint_ptr)g, r.x - m_scrollX, r.y - m_scrollY, &pos);
    }
    pthread_mutex_unlock(&m_renderMutex);
}

void WidgetWebview::onResize()
{
    m_clientWidth = area.w;
    m_clientHeight = area.h;
    dragStep = area.h / 4;

    pthread_mutex_lock(&m_renderMutex);
    if (m_container) {
        m_container->set_client_size(m_clientWidth, m_clientHeight);
    }
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

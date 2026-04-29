// Container for litehtml rendering using xwin/graph API

#include "XContainer.h"
#include "WidgetWebview/WidgetWebview.h"
#include "el_input.h"

#include <iostream>
#include <graph/graph_ex.h>
#include <graph/graph_image.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/klog.h>
#include <x++/X.h>
#include <tinyhttpsc/tinyhttpsc.h>
#include <stdlib.h>
#include <string.h>

using namespace litehtml;
using namespace Ewok;

XContainer::XContainer(litehtml::context* html_context, WidgetWebview* webview)
{
    m_g = NULL;
    m_webview = webview;
    m_client_width = 640;
    m_client_height = 480;
}

XContainer::~XContainer(void)
{
    for (auto& pair : m_fonts) {
        if (pair.second.font) {
            font_free(pair.second.font);
        }
    }
    m_fonts.clear();

    for (auto& pair : m_images) {
        if (pair.second.image) {
            graph_free(pair.second.image);
        }
    }
    m_images.clear();
}

uint32_t XContainer::web_color_to_graph(const litehtml::web_color& c)
{
    return (c.alpha << 24) | (c.red << 16) | (c.green << 8) | c.blue;
}

litehtml::uint_ptr XContainer::create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm)
{
    std::string fontName = "system-cn";
    std::string key = "system-cn";

    if(weight >= 700) {
        fontName = "system-cn";
        key = "system-cn";
    }

    key += "-" + std::to_string(size) + "px";

    FontInfo fontInfo;
    fontInfo.font = NULL;
    fontInfo.size = size;

    if (m_fonts.find(key) != m_fonts.end()) {
        fontInfo = m_fonts[key];
    } else {
        fontInfo.font = font_new(fontName.c_str(), false);
        fontInfo.size = size;
        if (fontInfo.font == NULL) {
            m_fonts[key] = fontInfo;
            return 0;
        }
        m_fonts[key] = fontInfo;
    }

    if(fontInfo.font == NULL) {
        return 0;
    }

    if (fm) {
        face_info_t face;
        if (font_get_face(fontInfo.font, size, &face) == 0) {
            // FreeType metrics are in 26.6 fixed-point format, divide by 64 to get pixels
            const int FACE_PIXEL_DENT = 64;
            fm->ascent = face.ascender / FACE_PIXEL_DENT;
            fm->descent = face.descender / FACE_PIXEL_DENT;
            fm->height = face.height / FACE_PIXEL_DENT;

            uint32_t w, h;
            font_char_size('x', fontInfo.font, size, &w, &h);
            fm->x_height = h / FACE_PIXEL_DENT;
            fm->draw_spaces = italic == fontStyleItalic || decoration;
        }
    }

    return (uint_ptr)&m_fonts[key];
}

void XContainer::delete_font(litehtml::uint_ptr hFont)
{
    return;
}

int XContainer::text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont)
{
    FontInfo* fontInfo = (FontInfo*)hFont;

    if(!fontInfo || !fontInfo->font || !text) {
        return 0;
    }

    uint32_t w, h;
    font_text_size(text, fontInfo->font, fontInfo->size, &w, &h);
    return w;
}

void XContainer::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos)
{
    FontInfo* fontInfo = (FontInfo*)hFont;
    if (!fontInfo || !fontInfo->font || !text) {
        return;
    }

    graph_t* g = m_g;
    if (!g) {
        return;
    }

    uint32_t graph_color = web_color_to_graph(color);

    // Force opaque color if alpha is 0
    if ((graph_color >> 24) == 0) {
        graph_color = 0xFF000000 | (graph_color & 0xFFFFFF);
    }

    graph_draw_text_font(g, pos.x, pos.y, text, fontInfo->font, fontInfo->size, graph_color);
}

int XContainer::pt_to_px(int pt)
{
    return pt;
}

int XContainer::get_default_font_size() const
{
    return 16;
}

const litehtml::tchar_t* XContainer::get_default_font_name() const
{
    return _t("sans-serif");
}

void XContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    graph_t* g = m_g;
    if (!g)
        return;

    if (!marker.image.empty())
    {
    }
    else
    {
        uint32_t color = web_color_to_graph(marker.color);

        switch (marker.marker_type)
        {
        case litehtml::list_style_type_circle:
            graph_circle(g, marker.pos.x, marker.pos.y, marker.pos.width, 1, color);
            break;
        case litehtml::list_style_type_disc:
            graph_fill_circle(g, marker.pos.x, marker.pos.y, marker.pos.width, color);
            break;
        case litehtml::list_style_type_square:
            graph_fill_rect(g, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, color);
            break;
        }
    }
}

const std::string XContainer::getFullURL(const std::string& src, const std::string& baseurl) {
    std::string path;
    if(src.starts_with("file://") || 
            src.starts_with("http://") ||
            src.starts_with("https://")) {
        return src;
    }
    else if(src.starts_with("res://")) {
        path = "file:/";
        path += X::getResFullName(src.substr(6).c_str());
        return path;
    }
    else if(src.starts_with("//")) {
        // Protocol-relative URL: //example.com/path
        // Use https by default, or http if baseurl uses http
        if(baseurl.starts_with("http://")) {
            path = "http:" + src;
        } else {
            path = "https:" + src;
        }
        return path;
    }

    if (!baseurl.empty()) {
        if(baseurl[baseurl.length() - 1] != '/')
            path = baseurl + "/" + path;
        else
            path = baseurl + path;
    }
    return path;
}

uint8_t* XContainer::loadURL(const std::string& url, int* sz)
{
    uint8_t* ret = NULL;
    if(sz != NULL)
        *sz = 0;

    std::string full_url = getFullURL(url, "");
    if(full_url.starts_with("file://")) {
        std::string path = full_url.substr(6);
        if(!path.empty()) { //local file
            ret = vfs_readfile(path.c_str(), sz);
            return ret;
        }
    }
    else if(full_url.starts_with("http://") || full_url.starts_with("https://")) {
        // Use tinyhttpsc to fetch HTTP/HTTPS URL
        TinyHttpsRequest* request = NewHttpsRequest(full_url.c_str());
        if(request == NULL) {
            return NULL;
        }
        
        // Set timeout and max redirections
        HttpsRequestSetTimeout(request, 10000); // 10 seconds
        HttpsRequestSetMaxRedirections(request, 5);
        
        // Execute request
        TinyHttpsResponse* response = HttpsRequestFetch(request);
        HttpsRequestFree(request);
        
        if(response == NULL) {
            return NULL;
        }
        
        // Check for errors
        if(HttpsResponseError(response)) {
            HttpsResponseFree(response);
            return NULL;
        }
        
        // Check status code
        int status_code = HttpsResponseGetStatusCode(response);
        if(status_code != 200) {
            HttpsResponseFree(response);
            return NULL;
        }
        
        // Read response body first (this will populate the body size)
        int body_size = 0;
        const char* body = HttpsResponseReadBody(response, &body_size);
        if(body == NULL || body_size <= 0) {
            HttpsResponseFree(response);
            return NULL;
        }
        
        // Allocate memory and copy data
        ret = (uint8_t*)malloc(body_size+1);
        if(ret == NULL) {
            HttpsResponseFree(response);
            return NULL;
        }
        
        memcpy(ret, body, body_size);
        HttpsResponseFree(response);
        ret[body_size] = 0;

        if(sz != NULL)
            *sz = body_size;
        return ret;
    }
    return NULL;
}


void XContainer::load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready)
{
    if (src == NULL || src[0] == 0)
        return;

    std::string img_path = std::string(src);
    std::string base_url = std::string(baseurl);
    std::string full_url = getFullURL(img_path, base_url);
    if(full_url.empty())
        return; 
    
    auto it = m_images.find(full_url);
    if (it != m_images.end()) {
        it->second.ref_count++;
        return;
    }

    m_webview->addTask( { full_url,  HttpTask::TASK_IMAGE } );
}

bool XContainer::loadImageData(const std::string& url, uint8_t* data, int sz)
{
    if (data == NULL || sz <= 0 || url.empty())
        return false;

    graph_t* img = graph_image_new_from_data(GRAPH_IMAGE_TYPE_AUTO, data, sz);
    if (img == NULL) {
        return false;
    }

    ImageInfo info;
    info.image = img;
    info.ref_count = 1;
    m_images[url] = info;
    return true;
}

void XContainer::get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz)
{
    sz.width = 0;
    sz.height = 0;

    if (src == NULL || src[0] == 0)
        return;

    std::string img_path = std::string(src);
    std::string base_url = std::string(baseurl);
     std::string full_url = getFullURL(img_path, base_url);
    if(full_url.empty())
        return;
    
    auto it = m_images.find(full_url);
    if (it != m_images.end() && it->second.image != NULL) {
        sz.width = it->second.image->w;
        sz.height = it->second.image->h;
        return;
    }
    return;
}

void XContainer::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg)
{
    graph_t* g = m_g;
    if (!g)
        return;

    bool do_image = false;
    if (!bg.image.empty() && bg.image_size.width > 0 && bg.image_size.height > 0) {
        std::string full_url = getFullURL(bg.image, m_base_url);
        if (!full_url.empty()) {
            auto it = m_images.find(full_url);
            graph_t* img = NULL;
            if (it != m_images.end() && it->second.image != NULL) {
                img = it->second.image;
            } 

            if (img != NULL) {
                graph_blt_fit_alpha(img, 0, 0, img->w, img->h,
                    g, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height, 0xFF);
                return;
            }
        }
        do_image = true;
    }

    uint32_t color = web_color_to_graph(bg.color);
    if ((color >> 24) == 0) {
        color = 0xFF000000 | (color & 0xFFFFFF);
    }

    if(!do_image)
        graph_fill_rect(g, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height, color);
    else
        graph_rect(g, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height, color);
}

void XContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
    graph_t* g = m_g;
    if (!g)
        return;

    if (borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden) {
        uint32_t color = web_color_to_graph(borders.top.color);
        graph_rect(g, draw_pos.x, draw_pos.y, draw_pos.width, draw_pos.height, color);
    }
}

void XContainer::transform_text(litehtml::tstring& text, litehtml::text_transform tt)
{
}

void XContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y)
{
}

void XContainer::del_clip()
{
}

void XContainer::clear_images()
{
    for (auto& pair : m_images) {
        if (pair.second.image) {
            graph_free(pair.second.image);
        }
    }
    m_images.clear();
}

void XContainer::clear_inputs()
{
    for (auto* input : m_vecInput) {
        delete input;
    }
    m_vecInput.clear();
}

void XContainer::get_client_rect(litehtml::position& client) const
{
    client.width = m_client_width;
    client.height = m_client_height;
}

void XContainer::set_client_size(int width, int height)
{
    m_client_width = width;
    m_client_height = height;
}

void XContainer::on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el)
{
}

void XContainer::set_cursor(const litehtml::tchar_t* cursor)
{
}

void XContainer::import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl)
{
    //klog("import_css: url:%s baseurl:%s text:%s\n", url.c_str(), baseurl.c_str(), text.c_str());
    if (url.empty())
        return;

    std::string css_path = std::string(url);
    std::string base_url = std::string(baseurl);
    std::string full_url = getFullURL(css_path, base_url);
    if(full_url.empty())
        return;
    m_webview->addTask( { full_url,  HttpTask::TASK_CSS } );
}

void XContainer::set_caption(const litehtml::tchar_t* caption)
{
}

void XContainer::set_base_url(const litehtml::tchar_t* base_url)
{
    if (base_url != NULL) {
        m_base_url = std::string(base_url);
    } else {
        m_base_url.clear();
    }
}

litehtml::element* XContainer::create_element(const litehtml::tchar_t* tag_name,
                                    const litehtml::string_map& attributes,
                                    litehtml::document* doc)
{
    if (!t_strcasecmp(tag_name, _t("input"))) {
        auto iter = attributes.find(_t("type"));
        if (iter != attributes.end()) {
            if (!t_strcasecmp(iter->second.c_str(), _t("text"))) {
                auto input = new el_input(doc, this, HtmlInputType::TEXT);
                m_vecInput.push_back(input);
                return input;
            }
            else if (!t_strcasecmp(iter->second.c_str(), _t("button"))) {
                auto input = new el_input(doc, this, HtmlInputType::BUTTON);
                m_vecInput.push_back(input);
                return input;
            }
        }
    }
    return NULL;
}

void XContainer::get_media_features(litehtml::media_features& media) const
{
    litehtml::position client;
    get_client_rect(client);
    media.type       = litehtml::media_type_screen;
    media.width      = client.width;
    media.height     = client.height;
    media.device_width  = 640;
    media.device_height = 480;
    media.color     = 8;
    media.monochrome  = 0;
    media.color_index = 256;
    media.resolution  = 96;
}

void XContainer::get_language(litehtml::tstring& language, litehtml::tstring& culture) const
{
    language = _t("en");
    culture = _t("");
}

void XContainer::link(litehtml::document* ptr, const litehtml::element::ptr& el)
{
}

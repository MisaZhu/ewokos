// Container for litehtml rendering using xwin/graph API

#include "XContainer.h"
#include "WidgetWebview/WidgetWebview.h"
#include "el_input.h"

#include <iostream>
#include <graph/graph_ex.h>
#include <ewoksys/basic_math.h>

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
}

uint32_t XContainer::web_color_to_graph(const litehtml::web_color& c)
{
    return (c.alpha << 24) | (c.red << 16) | (c.green << 8) | c.blue;
}

litehtml::uint_ptr XContainer::create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm)
{
    std::string fontName = "system";
    std::string key = "system";

    if(weight >= 700) {
        fontName = "system";
        key = "system";
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

void XContainer::load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready)
{
}

void XContainer::get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz)
{
    sz.width = 0;
    sz.height = 0;
}

void XContainer::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg)
{
    graph_t* g = m_g;
    if (!g)
        return;

    uint32_t color = web_color_to_graph(bg.color);

    // Force opaque color if alpha is 0
    if ((color >> 24) == 0) {
        color = 0xFF000000 | (color & 0xFFFFFF);
    }

    graph_fill_rect(g, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height, color);
}

void XContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
    graph_t* g = m_g;
    if (!g)
        return;

    if (borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden) {
        uint32_t color = web_color_to_graph(borders.top.color);
        graph_set(g, draw_pos.x, draw_pos.y, draw_pos.width, draw_pos.height, color);
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
}

void XContainer::set_caption(const litehtml::tchar_t* caption)
{
}

void XContainer::set_base_url(const litehtml::tchar_t* base_url)
{
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

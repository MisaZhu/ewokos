// Container for litehtml rendering using xwin/graph API

#pragma once

#include <litehtml.h>
#include <graph/graph.h>
#include <font/font.h>
#include <unordered_map>

class el_input;

namespace Ewok {
    class WidgetWebview;
}

// Store font with its size
struct FontInfo {
    font_t* font;
    int size;
};

// Store image data
struct ImageInfo {
    graph_t* image;
    int ref_count;
};

class XContainer : public litehtml::document_container {
public:
    XContainer(litehtml::context* html_context, Ewok::WidgetWebview* webview);
    virtual ~XContainer(void);

    virtual litehtml::uint_ptr         create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
    virtual void                       delete_font(litehtml::uint_ptr hFont) override;
    virtual int                         text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont) override;
    virtual void                       draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;

    virtual int                        pt_to_px(int pt) override;
    virtual int                        get_default_font_size() const override;
    virtual const litehtml::tchar_t*   get_default_font_name() const override;
    virtual void                       draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    virtual void                       load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready) override;
    virtual void                       get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) override;
    virtual void                       draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg) override;
    virtual void                       draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;

    virtual void                       transform_text(litehtml::tstring& text, litehtml::text_transform tt) override;
    virtual void                       set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y) override;
    virtual void                       del_clip() override;
    virtual litehtml::element*  create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, litehtml::document* doc) override;
    virtual void                       get_media_features(litehtml::media_features& media) const override;
    virtual void                       get_language(litehtml::tstring& language, litehtml::tstring& culture) const override;
    virtual void                       link(litehtml::document* doc, const litehtml::element::ptr& el) override;

    void                               clear_images();

    void                               get_client_rect(litehtml::position& client) const;
    void                               set_client_size(int width, int height);
    void                               on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el);
    void                               set_cursor(const litehtml::tchar_t* cursor);
    void                               import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl);
    void                               set_caption(const litehtml::tchar_t* caption);
    void                               set_base_url(const litehtml::tchar_t* base_url);

    void                               setGraph(graph_t* g) { m_g = g; }
    static uint8_t*                    loadURL(const std::string& url, int* sz);

private:
    graph_t* m_g;
    Ewok::WidgetWebview* m_webview;
    std::unordered_map<std::string, FontInfo> m_fonts;
    std::unordered_map<std::string, ImageInfo> m_images;
    int m_client_width;
    int m_client_height;
    std::string m_base_url;

    std::vector<el_input*> m_vecInput;

    static uint32_t web_color_to_graph(const litehtml::web_color& c);
    static const std::string getFullURL(const std::string& src, const std::string& baseurl);
};

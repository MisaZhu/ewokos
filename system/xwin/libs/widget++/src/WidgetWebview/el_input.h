// Input element for litehtml using xwin/graph API

#pragma once

#include <litehtml.h>
#include <graph/graph.h>

class XContainer;

enum HtmlInputType {
    TEXT,
    BUTTON
};

class el_input : public litehtml::html_tag
{
public:
    el_input(
        litehtml::document* doc,
        XContainer* container,
        HtmlInputType inputType);
    virtual ~el_input(void);

    virtual int      line_height() const override;
    virtual bool     is_replaced() const override;
    virtual void     get_content_size(litehtml::size& sz, int max_width) override;
    virtual litehtml::style_display    get_display() const override;
    virtual litehtml::element_position get_element_position(litehtml::css_offsets* offsets = 0) const override;
    virtual int      render(int x, int y, int max_width, bool second_pass = false) override;
    virtual void     draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip) override;
    virtual void     parse_styles(bool is_reparse) override;
    virtual void     on_click();

private:
    XContainer* m_container;
    HtmlInputType m_inputType;

    static uint32_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
};
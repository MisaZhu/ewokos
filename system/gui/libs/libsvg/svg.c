#include "svg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define SVG_DEBUG 1
#if SVG_DEBUG
#define SVG_DBG(fmt, ...) do { fprintf(stderr, "[SVG DBG] " fmt, ##__VA_ARGS__); } while(0)
#else
#define SVG_DBG(fmt, ...) do {} while(0)
#endif

#define SVG_MAX_WIDTH 4096
#define SVG_MAX_HEIGHT 4096

typedef struct {
    const char* data;
    uint32_t size;
    uint32_t pos;
} svg_parser_t;

static uint32_t svg_parse_color(const char* color_str);
static int svg_parse_length(const char* length_str);
static void svg_set_pixel(uint32_t* pixels, int img_width, int img_height,
                         int x, int y, uint32_t color);
static void svg_draw_line(uint32_t* pixels, int img_width, int img_height,
                         int x1, int y1, int x2, int y2, int width, uint32_t color);
static void svg_scanline_fill_polygon(uint32_t* pixels, int img_width, int img_height,
                                     int* xpts, int* ypts, int num_pts, uint32_t fill_color);
static void svg_bezier_draw(uint32_t* pixels, int img_width, int img_height,
                          float x0, float y0, float x1, float y1,
                          float x2, float y2, int width, uint32_t color);

static void svg_parser_skip_whitespace(svg_parser_t* parser) {
    while (parser->pos < parser->size && (parser->data[parser->pos] == ' ' ||
           parser->data[parser->pos] == '\t' || parser->data[parser->pos] == '\n' ||
           parser->data[parser->pos] == '\r')) {
        parser->pos++;
    }
}

static void svg_parser_skip_comment(svg_parser_t* parser) {
    if (parser->pos + 4 < parser->size &&
        strncmp(&parser->data[parser->pos], "<!--", 4) == 0) {
        parser->pos += 4;
        while (parser->pos + 3 < parser->size) {
            if (parser->data[parser->pos] == '-' && parser->data[parser->pos + 1] == '-' &&
                parser->data[parser->pos + 2] == '>') {
                parser->pos += 3;
                return;
            }
            parser->pos++;
        }
        parser->pos = parser->size;
    }
}

static bool svg_parser_read_name(svg_parser_t* parser, char* name, uint32_t max_len) {
    svg_parser_skip_whitespace(parser);
    uint32_t start = parser->pos;
    while (parser->pos < parser->size && parser->data[parser->pos] != '>' &&
           parser->data[parser->pos] != ' ' && parser->data[parser->pos] != '\t' &&
           parser->data[parser->pos] != '/' &&
           parser->data[parser->pos] != '\n' && parser->data[parser->pos] != '\r') {
        if (parser->data[parser->pos] >= 'A' && parser->data[parser->pos] <= 'Z') {
            name[parser->pos - start] = parser->data[parser->pos] + 32;
        } else {
            name[parser->pos - start] = parser->data[parser->pos];
        }
        parser->pos++;
        if (parser->pos - start >= max_len - 1) break;
    }
    name[parser->pos - start] = '\0';
    return (parser->pos - start) > 0;
}

static bool svg_parser_read_attr(svg_parser_t* parser, char* name, char* value, uint32_t max_len) {
    SVG_DBG("svg_parser_read_attr: ENTRY pos=%u, size=%u, char='%c'\n",
            parser->pos, parser->size, parser->data[parser->pos]);
    svg_parser_skip_whitespace(parser);
    SVG_DBG("svg_parser_read_attr: after skip whitespace pos=%u, char='%c'\n",
            parser->pos, parser->data[parser->pos]);
    if (parser->pos >= parser->size || parser->data[parser->pos] == '>' ||
        parser->data[parser->pos] == '/') {
        SVG_DBG("svg_parser_read_attr: returning false at pos %u, char='%c'\n",
                parser->pos, parser->data[parser->pos]);
        return false;
    }

    uint32_t name_start = parser->pos;
    while (parser->pos < parser->size && parser->data[parser->pos] != '=' &&
           parser->data[parser->pos] != ' ' && parser->data[parser->pos] != '\t') {
        if (parser->data[parser->pos] >= 'A' && parser->data[parser->pos] <= 'Z') {
            name[parser->pos - name_start] = parser->data[parser->pos] + 32;
        } else {
            name[parser->pos - name_start] = parser->data[parser->pos];
        }
        parser->pos++;
        if (parser->pos - name_start >= max_len - 1) break;
    }
    name[parser->pos - name_start] = '\0';

    if (parser->pos >= parser->size || parser->data[parser->pos] != '=') {
        SVG_DBG("svg_parser_read_attr: no '=' found at pos %u, char='%c'\n",
                parser->pos, parser->data[parser->pos]);
        return false;
    }
    parser->pos++;

    svg_parser_skip_whitespace(parser);

    char quote = 0;
    if (parser->pos < parser->size && (parser->data[parser->pos] == '"' ||
           parser->data[parser->pos] == '\'')) {
        quote = parser->data[parser->pos++];
    }

    uint32_t value_idx = 0;
    bool truncated = false;
    SVG_DBG("svg_parser_read_attr: value_start=%u, quote='%c', max_len=%u\n", parser->pos, quote, max_len);
    while (parser->pos < parser->size) {
        if (quote != 0) {
            if (parser->data[parser->pos] == quote) {
                SVG_DBG("svg_parser_read_attr: found closing quote at pos %u\n", parser->pos);
                break;
            }
        } else {
            if (parser->data[parser->pos] == ' ' || parser->data[parser->pos] == '\t' ||
                parser->data[parser->pos] == '>' || parser->data[parser->pos] == '/') {
                break;
            }
        }
        if (value_idx < max_len - 1) {
            value[value_idx++] = parser->data[parser->pos];
        }
        parser->pos++;
        if (value_idx >= max_len - 1) {
            truncated = true;
            SVG_DBG("svg_parser_read_attr: TRUNCATED! pos=%u, value_idx=%u, max_len=%u\n",
                    parser->pos, value_idx, max_len);
            break;
        }
    }
    value[value_idx] = '\0';
    SVG_DBG("svg_parser_read_attr: value read len=%u, truncated=%d\n", value_idx, truncated);

    if (truncated && quote == 0) {
        while (parser->pos < parser->size) {
            if (parser->data[parser->pos] == ' ' || parser->data[parser->pos] == '\t' ||
                parser->data[parser->pos] == '>' || parser->data[parser->pos] == '/') {
                break;
            }
            parser->pos++;
        }
        SVG_DBG("svg_parser_read_attr: truncated unquoted, skipping to pos %u char='%c'\n",
                parser->pos, parser->data[parser->pos]);
        return false;
    } else if (truncated && quote != 0) {
        SVG_DBG("svg_parser_read_attr: truncated quoted, skipping to find quote\n");
        while (parser->pos < parser->size) {
            if (parser->data[parser->pos] == quote) {
                parser->pos++;
                SVG_DBG("svg_parser_read_attr: found closing quote at pos %u\n", parser->pos);
                break;
            }
            parser->pos++;
        }
        SVG_DBG("svg_parser_read_attr: truncated quoted, now at pos %u char='%c'\n",
                parser->pos, parser->data[parser->pos]);
        return false;
    }

    if (quote != 0 && parser->pos < parser->size && !truncated) {
        parser->pos++;
    }

    SVG_DBG("svg_parser_read_attr: name='%s' value='%s' quote=%c\n", name, value, quote);
    return true;
}

static void svg_parse_style(const char* style, uint32_t* fill, uint32_t* stroke, int* stroke_width) {
    if (style == NULL || style[0] == '\0') return;

    char* style_copy = (char*)malloc(strlen(style) + 1);
    if (!style_copy) return;
    strcpy(style_copy, style);

    char* saveptr;
    char* token = strtok_r(style_copy, ";", &saveptr);
    while (token != NULL) {
        while (*token == ' ' || *token == '\t') token++;

        char* colon = strchr(token, ':');
        if (colon != NULL) {
            *colon = '\0';
            char* prop_name = token;
            char* prop_value = colon + 1;
            while (*prop_value == ' ' || *prop_value == '\t') prop_value++;

            if (strcmp(prop_name, "fill") == 0) {
                *fill = svg_parse_color(prop_value);
            } else if (strcmp(prop_name, "stroke") == 0) {
                *stroke = svg_parse_color(prop_value);
            } else if (strcmp(prop_name, "stroke-width") == 0) {
                *stroke_width = svg_parse_length(prop_value);
            }
        }
        token = strtok_r(NULL, ";", &saveptr);
    }

    free(style_copy);
}

typedef struct {
    float trans_x;
    float trans_y;
    float angle;
} svg_transform_t;

static void svg_parse_transform(const char* transform_str, svg_transform_t* t) {
    t->trans_x = 0;
    t->trans_y = 0;
    t->angle = 0;

    if (transform_str == NULL || transform_str[0] == '\0') return;

    SVG_DBG("svg_parse_transform: input='%s'\n", transform_str);

    // Parse translate(x, y)
    const char* p = strstr(transform_str, "translate(");
    if (p) {
        p += 10; // skip "translate("
        char* end;
        t->trans_x = strtof(p, &end);
        p = end;
        while (*p == ' ' || *p == ',') p++;
        t->trans_y = strtof(p, &end);
    }

    // Parse rotate(angle)
    p = strstr(transform_str, "rotate(");
    if (p) {
        p += 7; // skip "rotate("
        t->angle = strtof(p, NULL);
    }

    SVG_DBG("svg_parse_transform: trans_x=%f, trans_y=%f, angle=%f\n", t->trans_x, t->trans_y, t->angle);
}

static void svg_apply_transform(float* x, float* y, const svg_transform_t* t) {
    float angle_rad = t->angle * 3.14159265f / 180.0f;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);
    float rx = *x * cos_a - *y * sin_a;
    float ry = *x * sin_a + *y * cos_a;
    *x = rx + t->trans_x;
    *y = ry + t->trans_y;
}

static uint32_t svg_parse_color(const char* color_str) {
    if (color_str == NULL || color_str[0] == '\0') {
        return 0xFF000000;
    }

    if (color_str[0] == '#') {
        if (strlen(color_str) == 4) {
            char r = color_str[1];
            char g = color_str[2];
            char b = color_str[3];
            char buf[3] = {r, r, 0};
            uint32_t rv = strtol(buf, NULL, 16);
            buf[0] = g; buf[1] = g;
            uint32_t gv = strtol(buf, NULL, 16);
            buf[0] = b; buf[1] = b;
            uint32_t bv = strtol(buf, NULL, 16);
            return 0xFF000000 | (rv << 16) | (gv << 8) | bv;
        } else if (strlen(color_str) == 7) {
            uint32_t color = strtol(color_str + 1, NULL, 16);
            return 0xFF000000 | color;
        }
    }

    if (strcmp(color_str, "black") == 0) return 0xFF000000;
    if (strcmp(color_str, "white") == 0) return 0xFFFFFFFF;
    if (strcmp(color_str, "red") == 0) return 0xFFFF0000;
    if (strcmp(color_str, "green") == 0) return 0xFF00FF00;
    if (strcmp(color_str, "blue") == 0) return 0xFF0000FF;
    if (strcmp(color_str, "yellow") == 0) return 0xFFFFFF00;
    if (strcmp(color_str, "cyan") == 0) return 0xFF00FFFF;
    if (strcmp(color_str, "magenta") == 0) return 0xFFFF00FF;
    if (strcmp(color_str, "gray") == 0 || strcmp(color_str, "grey") == 0) return 0xFF808080;
    if (strcmp(color_str, "orange") == 0) return 0xFFFFA500;
    if (strcmp(color_str, "purple") == 0) return 0xFF800080;
    if (strcmp(color_str, "pink") == 0) return 0xFFFFC0CB;
    if (strcmp(color_str, "brown") == 0) return 0xFFA52A2A;
    if (strcmp(color_str, "navy") == 0) return 0xFF000080;
    if (strcmp(color_str, "teal") == 0) return 0xFF008080;
    if (strcmp(color_str, "olive") == 0) return 0xFF808000;
    if (strcmp(color_str, "maroon") == 0) return 0xFF800000;
    if (strcmp(color_str, "silver") == 0) return 0xFFC0C0C0;
    if (strcmp(color_str, "lime") == 0) return 0xFF00FF00;
    if (strcmp(color_str, "aqua") == 0) return 0xFF00FFFF;
    if (strcmp(color_str, "fuchsia") == 0) return 0xFFFF00FF;
    if (strcmp(color_str, "transparent") == 0) return 0x00000000;
    if (strcmp(color_str, "none") == 0) return 0x00000000;

    return strtol(color_str, NULL, 16) | 0xFF000000;
}

static int svg_parse_length(const char* length_str) {
    if (length_str == NULL || length_str[0] == '\0') {
        return 0;
    }

    char* end;
    double value = strtod(length_str, &end);

    while (*end != '\0' && (*end == ' ' || *end == '\t')) {
        end++;
    }

    if (*end == 'p' && *(end + 1) == 'x') {
        return (int)(value + 0.5);
    } else if (*end == 'p' && *(end + 1) == 't') {
        return (int)(value * 1.3333 + 0.5);
    } else if (*end == 'p' && *(end + 1) == 'c') {
        return (int)(value * 12 + 0.5);
    } else if (*end == 'm' && *(end + 1) == 'm') {
        return (int)(value * 3.779528 + 0.5);
    } else if (*end == 'c' && *(end + 1) == 'm') {
        return (int)(value * 37.79528 + 0.5);
    } else if (*end == 'i' && *(end + 1) == 'n') {
        return (int)(value * 96 + 0.5);
    } else if (*end == '%') {
        return (int)(value + 0.5);
    }

    return (int)(value + 0.5);
}

static void svg_set_pixel(uint32_t* pixels, int img_width, int img_height,
                         int x, int y, uint32_t color) {
    if (x < 0 || x >= img_width || y < 0 || y >= img_height) return;
    if (color == 0x00000000) return;

    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    uint32_t old = pixels[y * img_width + x];
    uint8_t oa = (old >> 24) & 0xFF;
    uint8_t or = (old >> 16) & 0xFF;
    uint8_t og = (old >> 8) & 0xFF;
    uint8_t ob = old & 0xFF;

    if (a == 0xFF) {
        pixels[y * img_width + x] = color;
    } else if (a > 0) {
        uint8_t na = oa + (uint8_t)((255 - oa) * a / 255);
        uint8_t nr = (uint8_t)((r * a + or * (255 - a)) / 255);
        uint8_t ng = (uint8_t)((g * a + og * (255 - a)) / 255);
        uint8_t nb = (uint8_t)((b * a + ob * (255 - a)) / 255);
        pixels[y * img_width + x] = (na << 24) | (nr << 16) | (ng << 8) | nb;
    }
}

static void svg_draw_line(uint32_t* pixels, int img_width, int img_height,
                         int x1, int y1, int x2, int y2, int width,
                         uint32_t stroke_color) {
    if (stroke_color == 0x00000000 || width <= 0) return;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (1) {
        for (int wy = -width / 2; wy <= width / 2; wy++) {
            for (int wx = -width / 2; wx <= width / 2; wx++) {
                svg_set_pixel(pixels, img_width, img_height, x1 + wx, y1 + wy, stroke_color);
            }
        }

        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

static void svg_draw_rect(uint32_t* pixels, int img_width, int img_height,
                          int x, int y, int w, int h,
                          uint32_t fill_color) {
    if (fill_color == 0x00000000) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > img_width) { w = img_width - x; }
    if (y + h > img_height) { h = img_height - y; }
    if (w <= 0 || h <= 0) return;

    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            svg_set_pixel(pixels, img_width, img_height, px, py, fill_color);
        }
    }
}

static void svg_draw_rotated_rect(uint32_t* pixels, int img_width, int img_height,
                                   int x, int y, int w, int h,
                                   float trans_x, float trans_y, float angle,
                                   uint32_t fill_color, float scale_x, float scale_y,
                                   int vb_x, int vb_y) {
    if (fill_color == 0x00000000) return;

    float angle_rad = angle * 3.14159265f / 180.0f;
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    // Transform rect corners: apply transform in viewBox space first, then scale to pixels
    float corners[4][2];
    float raw_corners[4][2] = {
        {(float)x, (float)y},
        {(float)(x + w), (float)y},
        {(float)(x + w), (float)(y + h)},
        {(float)x, (float)(y + h)}
    };

    for (int i = 0; i < 4; i++) {
        // SVG transform order: right-to-left
        // translate(T) rotate(R) => P' = R(P) + T
        float px = raw_corners[i][0];
        float py = raw_corners[i][1];
        // Rotate first
        float rx = px * cos_a - py * sin_a;
        float ry = px * sin_a + py * cos_a;
        // Then translate
        rx += trans_x;
        ry += trans_y;
        // Scale to pixel coordinates
        corners[i][0] = (rx - vb_x) * scale_x;
        corners[i][1] = (ry - vb_y) * scale_y;
    }

    SVG_DBG("svg_draw_rotated_rect: raw=(%d,%d,%d,%d) trans=(%f,%f) angle=%f\n",
            x, y, w, h, trans_x, trans_y, angle);
    SVG_DBG("svg_draw_rotated_rect: corners[0]=(%f,%f) corners[2]=(%f,%f)\n",
            corners[0][0], corners[0][1], corners[2][0], corners[2][1]);

    // Fill rotated rect using scanline - fixed edge cases
    float miny_f = corners[0][1];
    float maxy_f = corners[0][1];
    for (int i = 1; i < 4; i++) {
        if (corners[i][1] < miny_f) miny_f = corners[i][1];
        if (corners[i][1] > maxy_f) maxy_f = corners[i][1];
    }

    int miny = (int)(miny_f - 0.5f);
    int maxy = (int)(maxy_f + 0.5f);
    if (miny < 0) miny = 0;
    if (maxy >= img_height) maxy = img_height - 1;

    for (int py = miny; py <= maxy; py++) {
        float scan_y = (float)py + 0.5f;
        float x_intersects[8];
        int num_intersect = 0;

        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            float y1 = corners[i][1];
            float y2 = corners[j][1];

            // Check if scan line intersects this edge
            if ((y1 < scan_y && y2 >= scan_y) || (y2 < scan_y && y1 >= scan_y)) {
                float t = (scan_y - y1) / (y2 - y1);
                x_intersects[num_intersect++] = corners[i][0] + t * (corners[j][0] - corners[i][0]);
            }
        }

        // Sort intersections
        for (int i = 0; i < num_intersect - 1; i++) {
            for (int j = 0; j < num_intersect - i - 1; j++) {
                if (x_intersects[j] > x_intersects[j + 1]) {
                    float tmp = x_intersects[j];
                    x_intersects[j] = x_intersects[j + 1];
                    x_intersects[j + 1] = tmp;
                }
            }
        }

        // Fill between pairs
        for (int i = 0; i < num_intersect - 1; i += 2) {
            int x_start = (int)(x_intersects[i] - 0.5f);
            int x_end = (int)(x_intersects[i + 1] + 0.5f);
            if (x_start < 0) x_start = 0;
            if (x_end >= img_width) x_end = img_width - 1;
            for (int px = x_start; px <= x_end; px++) {
                svg_set_pixel(pixels, img_width, img_height, px, py, fill_color);
            }
        }
    }
}

static void svg_draw_circle(uint32_t* pixels, int img_width, int img_height,
                             int cx, int cy, int radius, uint32_t fill_color) {
    if (fill_color == 0x00000000 || radius <= 0) return;

    int r2 = radius * radius;

    for (int py = cy - radius; py <= cy + radius; py++) {
        for (int px = cx - radius; px <= cx + radius; px++) {
            int dx = px - cx;
            int dy = py - cy;
            if (dx * dx + dy * dy <= r2) {
                svg_set_pixel(pixels, img_width, img_height, px, py, fill_color);
            }
        }
    }
}

static void svg_scanline_fill_polygon(uint32_t* pixels, int img_width, int img_height,
                                     int* xpts, int* ypts, int num_pts, uint32_t fill_color) {
    if (num_pts < 3) return;

    int miny = ypts[0];
    int maxy = ypts[0];
    for (int i = 1; i < num_pts; i++) {
        if (ypts[i] < miny) miny = ypts[i];
        if (ypts[i] > maxy) maxy = ypts[i];
    }

    if (miny < 0) miny = 0;
    if (maxy >= img_height) maxy = img_height - 1;

    for (int y = miny; y <= maxy; y++) {
        int intersections[64];
        int num_intersections = 0;

        for (int i = 0; i < num_pts; i++) {
            int j = (i + 1) % num_pts;
            int yi = ypts[i];
            int yj = ypts[j];

            if ((yi <= y && y < yj) || (yj <= y && y < yi)) {
                float x = xpts[i] + (float)(xpts[j] - xpts[i]) * (y - yi) / (float)(yj - yi);
                if (num_intersections < 64) {
                    intersections[num_intersections++] = (int)(x + 0.5f);
                }
            }
        }

        for (int i = 0; i < num_intersections - 1; i++) {
            for (int j = i + 1; j < num_intersections; j++) {
                if (intersections[i] > intersections[j]) {
                    int temp = intersections[i];
                    intersections[i] = intersections[j];
                    intersections[j] = temp;
                }
            }
        }

        for (int i = 0; i < num_intersections; i += 2) {
            if (i + 1 >= num_intersections) break;
            int x1 = intersections[i];
            int x2 = intersections[i + 1];

            if (x1 < 0) x1 = 0;
            if (x2 >= img_width) x2 = img_width - 1;

            for (int x = x1; x <= x2; x++) {
                svg_set_pixel(pixels, img_width, img_height, x, y, fill_color);
            }
        }
    }
}

static void svg_draw_polygon(uint32_t* pixels, int img_width, int img_height,
                             const char* points_str, uint32_t fill_color,
                             uint32_t stroke_color, int stroke_width) {
    if (points_str == NULL || points_str[0] == '\0') return;

    int xpts[128];
    int ypts[128];
    int num_pts = 0;

    char* points_copy = (char*)malloc(strlen(points_str) + 1);
    if (!points_copy) return;
    strcpy(points_copy, points_str);

    char* p = points_copy;
    while (*p && num_pts < 128) {
        while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\n' || *p == '\r') p++;
        if (*p == '\0') break;

        char* end;
        double x = strtod(p, &end);
        if (end == p) break;
        p = end;

        while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\n' || *p == '\r') p++;
        if (*p == '\0') break;

        double y = strtod(p, &end);
        if (end == p) break;
        p = end;

        xpts[num_pts] = (int)(x + 0.5);
        ypts[num_pts] = (int)(y + 0.5);
        num_pts++;
    }

    free(points_copy);

    if (num_pts < 3) return;

    if (fill_color != 0x00000000) {
        svg_scanline_fill_polygon(pixels, img_width, img_height, xpts, ypts, num_pts, fill_color);
    }

    if (stroke_width > 0 && stroke_color != 0x00000000) {
        for (int i = 0; i < num_pts; i++) {
            int j = (i + 1) % num_pts;
            svg_draw_line(pixels, img_width, img_height, xpts[i], ypts[i], xpts[j], ypts[j], stroke_width, stroke_color);
        }
    }
}

static float svg_bezier_point(float t, float p0, float p1, float p2) {
    float mt = 1.0f - t;
    return mt * mt * p0 + 2 * mt * t * p1 + t * t * p2;
}

static float svg_bezier_cubic_point(float t, float p0, float p1, float p2, float p3) {
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = t * t;
    float t3 = t2 * t;
    return mt3 * p0 + 3 * mt2 * t * p1 + 3 * mt * t2 * p2 + t3 * p3;
}

static void svg_bezier_draw(uint32_t* pixels, int img_width, int img_height,
                          float x0, float y0, float x1, float y1,
                          float x2, float y2, int width, uint32_t color) {
    if (width <= 0) return;

    int num_steps = 100;
    if (width > 2) num_steps = 200;

    float prev_x = x0;
    float prev_y = y0;

    for (int i = 1; i <= num_steps; i++) {
        float t = (float)i / num_steps;
        float x = svg_bezier_point(t, x0, x1, x2);
        float y = svg_bezier_point(t, y0, y1, y2);

        int ix0 = (int)(prev_x + 0.5f);
        int iy0 = (int)(prev_y + 0.5f);
        int ix1 = (int)(x + 0.5f);
        int iy1 = (int)(y + 0.5f);

        svg_draw_line(pixels, img_width, img_height, ix0, iy0, ix1, iy1, width, color);

        prev_x = x;
        prev_y = y;
    }
}

static void svg_bezier_cubic_draw(uint32_t* pixels, int img_width, int img_height,
                                  float x0, float y0, float x1, float y1,
                                  float x2, float y2, float x3, float y3,
                                  int width, uint32_t color) {
    if (width <= 0) return;

    int num_steps = 100;
    if (width > 2) num_steps = 200;

    float prev_x = x0;
    float prev_y = y0;

    for (int i = 1; i <= num_steps; i++) {
        float t = (float)i / num_steps;
        float x = svg_bezier_cubic_point(t, x0, x1, x2, x3);
        float y = svg_bezier_cubic_point(t, y0, y1, y2, y3);

        int ix0 = (int)(prev_x + 0.5f);
        int iy0 = (int)(prev_y + 0.5f);
        int ix1 = (int)(x + 0.5f);
        int iy1 = (int)(y + 0.5f);

        svg_draw_line(pixels, img_width, img_height, ix0, iy0, ix1, iy1, width, color);

        prev_x = x;
        prev_y = y;
    }
}

static void svg_draw_path(uint32_t* pixels, int img_width, int img_height,
                         const char* path_data, uint32_t fill_color,
                         uint32_t stroke_color, int stroke_width,
                         float scale_x, float scale_y, int vb_x, int vb_y) {
    char* path_copy = strdup(path_data);
    if (!path_copy) return;

    int start_x = 0, start_y = 0;
    int cur_x = 0, cur_y = 0;
    int last_cx = 0, last_cy = 0;

    int path_points[2048];
    int path_y_points[2048];
    int path_point_count = 0;

    char* p = path_copy;
    char command = 0;
    bool first_command = true;

    while (*p) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\n' || *p == '\r') p++;

        if (*p == '\0') break;

        // Check for new command
        if (*p == 'M' || *p == 'm' || *p == 'L' || *p == 'l' ||
            *p == 'C' || *p == 'c' || *p == 'Q' || *p == 'q' ||
            *p == 'S' || *p == 's' || *p == 'z' || *p == 'Z') {
            command = *p;
            p++;
            first_command = true;
        }

        if (command == 'z' || command == 'Z') {
            if (cur_x != start_x || cur_y != start_y) {
                svg_draw_line(pixels, img_width, img_height,
                            cur_x, cur_y, start_x, start_y,
                            stroke_width > 0 ? stroke_width : 1, stroke_color);
            }
            if (path_point_count >= 3 && fill_color != 0x00000000) {
                svg_scanline_fill_polygon(pixels, img_width, img_height,
                                        path_points, path_y_points, path_point_count, fill_color);
            }
            path_point_count = 0;
            cur_x = start_x;
            cur_y = start_y;
            continue;
        }

        // Determine how many values we need for this command
        int max_values = 6;
        if (command == 'C' || command == 'c' || command == 'S' || command == 's') {
            max_values = 6;
        } else if (command == 'Q' || command == 'q') {
            max_values = 4;
        } else if (command == 'L' || command == 'l' || command == 'M' || command == 'm') {
            max_values = 2;
        }

        // Read values for this command - SVG allows implicit command repetition
        // so we keep reading values until we hit a new command or end of string
        float values[8];
        int num_values = 0;

        while (num_values < max_values && *p != '\0') {
            // Skip whitespace and commas
            while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\n' || *p == '\r') p++;
            if (*p == '\0') break;

            // Check if this is a new command (not a number)
            if (*p == 'M' || *p == 'm' || *p == 'L' || *p == 'l' ||
                *p == 'C' || *p == 'c' || *p == 'Q' || *p == 'q' ||
                *p == 'S' || *p == 's' || *p == 'z' || *p == 'Z') {
                // New command found, break to let outer loop handle it
                break;
            }

            // Check if this is a number (digit, decimal point, or minus sign followed by digit)
            bool is_number = false;
            if (*p == '-') {
                // Minus sign indicates a negative number if followed by a digit or decimal point
                if ((*(p+1) >= '0' && *(p+1) <= '9') || *(p+1) == '.') {
                    is_number = true;
                }
            } else if ((*p >= '0' && *p <= '9') || *p == '.') {
                is_number = true;
            }

            if (is_number) {
                char* end;
                double val = strtod(p, &end);
                if (end == p) {
                    // Failed to parse, move forward to avoid infinite loop
                    p++;
                    break;
                }
                p = end;
                values[num_values++] = (float)val;
            } else {
                // Not a number and not a command we recognize, skip it
                p++;
                break;
            }
        }

        if (num_values == 0) continue;

        // If we didn't read enough values for this command, skip
        if (num_values < max_values) {
            continue;
        }

        SVG_DBG("svg_draw_path: command='%c', num_values=%d, values=[%f, %f, %f, %f, %f, %f]\n",
                command, num_values, values[0], values[1], num_values > 2 ? values[2] : 0,
                num_values > 3 ? values[3] : 0, num_values > 4 ? values[4] : 0, num_values > 5 ? values[5] : 0);

        // After M/m command, subsequent coordinate pairs are treated as L/l commands
        char effective_command = command;
        if (!first_command && (command == 'M' || command == 'm')) {
            effective_command = (command == 'M') ? 'L' : 'l';
        }
        first_command = false;

        if (effective_command == 'M' || effective_command == 'm') {
            float dx = values[0];
            float dy = values[1];
            if (effective_command == 'm') {
                cur_x += (int)(dx * scale_x + 0.5f);
                cur_y += (int)(dy * scale_y + 0.5f);
            } else {
                cur_x = (int)((dx - vb_x) * scale_x + 0.5f);
                cur_y = (int)((dy - vb_y) * scale_y + 0.5f);
            }
            start_x = cur_x;
            start_y = cur_y;
            path_point_count = 0;
            if (path_point_count < 2046) {
                path_points[path_point_count] = cur_x;
                path_y_points[path_point_count++] = cur_y;
            }
        }
        else if (effective_command == 'L' || effective_command == 'l') {
            int target_x, target_y;
            if (effective_command == 'l') {
                target_x = cur_x + (int)(values[0] * scale_x + 0.5f);
                target_y = cur_y + (int)(values[1] * scale_y + 0.5f);
            } else {
                target_x = (int)((values[0] - vb_x) * scale_x + 0.5f);
                target_y = (int)((values[1] - vb_y) * scale_y + 0.5f);
            }

            svg_draw_line(pixels, img_width, img_height,
                        cur_x, cur_y, target_x, target_y,
                        stroke_width > 0 ? stroke_width : 1, stroke_color);
            if (path_point_count < 2046) {
                path_points[path_point_count] = target_x;
                path_y_points[path_point_count++] = target_y;
            }
            cur_x = target_x;
            cur_y = target_y;
        }
        else if (effective_command == 'C' || effective_command == 'c') {
            float x1, y1, x2, y2, x3, y3;
            if (effective_command == 'c') {
                x1 = cur_x + values[0] * scale_x;
                y1 = cur_y + values[1] * scale_y;
                x2 = cur_x + values[2] * scale_x;
                y2 = cur_y + values[3] * scale_y;
                x3 = cur_x + values[4] * scale_x;
                y3 = cur_y + values[5] * scale_y;
            } else {
                x1 = (values[0] - vb_x) * scale_x;
                y1 = (values[1] - vb_y) * scale_y;
                x2 = (values[2] - vb_x) * scale_x;
                y2 = (values[3] - vb_y) * scale_y;
                x3 = (values[4] - vb_x) * scale_x;
                y3 = (values[5] - vb_y) * scale_y;
            }

            svg_bezier_cubic_draw(pixels, img_width, img_height,
                            (float)cur_x, (float)cur_y,
                            x1, y1, x2, y2, x3, y3,
                            stroke_width > 0 ? stroke_width : 1, stroke_color);

            // Add sample points along the curve for accurate polygon fill
            int num_samples = 20;
            for (int i = 1; i <= num_samples; i++) {
                float t = (float)i / num_samples;
                float sx = svg_bezier_cubic_point(t, (float)cur_x, x1, x2, x3);
                float sy = svg_bezier_cubic_point(t, (float)cur_y, y1, y2, y3);
                if (path_point_count < 2046) {
                    path_points[path_point_count] = (int)(sx + 0.5f);
                    path_y_points[path_point_count++] = (int)(sy + 0.5f);
                }
            }

            last_cx = (int)x2;
            last_cy = (int)y2;
            cur_x = (int)x3;
            cur_y = (int)y3;
        }
        else if (effective_command == 'S' || effective_command == 's') {
            float x1, y1, x2, y2, x3, y3;
            float refl_x = 2 * cur_x - last_cx;
            float refl_y = 2 * cur_y - last_cy;

            if (effective_command == 's') {
                x1 = refl_x;
                y1 = refl_y;
                x2 = cur_x + values[0] * scale_x;
                y2 = cur_y + values[1] * scale_y;
                x3 = cur_x + values[2] * scale_x;
                y3 = cur_y + values[3] * scale_y;
            } else {
                x1 = refl_x;
                y1 = refl_y;
                x2 = (values[0] - vb_x) * scale_x;
                y2 = (values[1] - vb_y) * scale_y;
                x3 = (values[2] - vb_x) * scale_x;
                y3 = (values[3] - vb_y) * scale_y;
            }

            svg_bezier_cubic_draw(pixels, img_width, img_height,
                            (float)cur_x, (float)cur_y,
                            x1, y1, x2, y2, x3, y3,
                            stroke_width > 0 ? stroke_width : 1, stroke_color);

            // Add sample points along the curve for accurate polygon fill
            int num_samples = 20;
            for (int i = 1; i <= num_samples; i++) {
                float t = (float)i / num_samples;
                float sx = svg_bezier_cubic_point(t, (float)cur_x, x1, x2, x3);
                float sy = svg_bezier_cubic_point(t, (float)cur_y, y1, y2, y3);
                if (path_point_count < 2046) {
                    path_points[path_point_count] = (int)(sx + 0.5f);
                    path_y_points[path_point_count++] = (int)(sy + 0.5f);
                }
            }

            last_cx = (int)x2;
            last_cy = (int)y2;
            cur_x = (int)x3;
            cur_y = (int)y3;
        }
        else if (effective_command == 'Q' || effective_command == 'q') {
            float x1, y1, x2, y2;
            if (effective_command == 'q') {
                x1 = cur_x + values[0] * scale_x;
                y1 = cur_y + values[1] * scale_y;
                x2 = cur_x + values[2] * scale_x;
                y2 = cur_y + values[3] * scale_y;
            } else {
                x1 = (values[0] - vb_x) * scale_x;
                y1 = (values[1] - vb_y) * scale_y;
                x2 = (values[2] - vb_x) * scale_x;
                y2 = (values[3] - vb_y) * scale_y;
            }

            float cx1 = (float)cur_x + 2.0f/3.0f * (x1 - (float)cur_x);
            float cy1 = (float)cur_y + 2.0f/3.0f * (y1 - (float)cur_y);
            float cx2 = x2 + 2.0f/3.0f * (x1 - x2);
            float cy2 = y2 + 2.0f/3.0f * (y1 - y2);

            svg_bezier_draw(pixels, img_width, img_height,
                            (float)cur_x, (float)cur_y,
                            cx1, cy1, cx2, cy2,
                            stroke_width > 0 ? stroke_width : 1, stroke_color);

            // Add sample points along the curve for accurate polygon fill
            int num_samples = 20;
            for (int i = 1; i <= num_samples; i++) {
                float t = (float)i / num_samples;
                float sx = svg_bezier_point(t, (float)cur_x, cx1, cx2);
                float sy = svg_bezier_point(t, (float)cur_y, cy1, cy2);
                if (path_point_count < 2046) {
                    path_points[path_point_count] = (int)(sx + 0.5f);
                    path_y_points[path_point_count++] = (int)(sy + 0.5f);
                }
            }

            last_cx = (int)x1;
            last_cy = (int)y1;
            cur_x = (int)x2;
            cur_y = (int)y2;
        }
    }

    free(path_copy);
}

svg_image_t* svg_load_from_memory(const uint8_t* data, uint32_t size) {
    SVG_DBG("svg_load_from_memory: start, size=%u\n", size);
    if (data == NULL || size == 0) {
        SVG_DBG("svg_load_from_memory: NULL data or zero size\n");
        return NULL;
    }

    svg_parser_t parser;
    parser.data = (const char*)data;
    parser.size = size;
    parser.pos = 0;

    int width = 0;
    int height = 0;

    while (parser.pos < parser.size) {
        svg_parser_skip_whitespace(&parser);
        svg_parser_skip_comment(&parser);
        svg_parser_skip_whitespace(&parser);

        if (parser.pos >= parser.size || parser.data[parser.pos] != '<') {
            SVG_DBG("svg_load_from_memory: break at pos %u, size %u\n", parser.pos, parser.size);
            break;
        }
        parser.pos++;

        if (parser.pos >= parser.size) break;

        if (parser.data[parser.pos] == '/') {
            parser.pos++;
            char name[64];
            svg_parser_read_name(&parser, name, sizeof(name));
            SVG_DBG("svg_load_from_memory: closing tag </%s>\n", name);
            continue;
        }

        char name[64];
        svg_parser_read_name(&parser, name, sizeof(name));
        SVG_DBG("svg_load_from_memory: tag <%s> at pos %u\n", name, parser.pos);

        if (strcmp(name, "?xml") == 0) {
            while (parser.pos < parser.size && parser.data[parser.pos] != '>') {
                parser.pos++;
            }
            if (parser.pos < parser.size) parser.pos++;
            continue;
        }

        if (strcmp(name, "svg") == 0) {
            char attr_name[64];
            char attr_value[8192];
            int viewbox_width = 0, viewbox_height = 0;
            bool has_viewbox = false;

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "width") == 0) {
                    width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "height") == 0) {
                    height = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "viewbox") == 0) {
                    char* saveptr;
                    char* token = strtok_r(attr_value, " ,", &saveptr);
                    if (token != NULL) { token = strtok_r(NULL, " ,", &saveptr); }  // skip x
                    if (token != NULL) { token = strtok_r(NULL, " ,", &saveptr); }  // skip y
                    if (token != NULL) { viewbox_width = svg_parse_length(token); } // width
                    if (token != NULL) { token = strtok_r(NULL, " ,", &saveptr); }
                    if (token != NULL) { viewbox_height = svg_parse_length(token); } // height
                    has_viewbox = true;
                }
            }

            // If viewBox is specified and differs from width/height, use viewBox dimensions
            // The actual scaling will be done during rendering
            if (has_viewbox && viewbox_width > 0 && viewbox_height > 0) {
                // Store viewBox dimensions for coordinate transformation
                // For now, just use viewBox as the coordinate system
                if (width <= 0) width = viewbox_width;
                if (height <= 0) height = viewbox_height;
            }

            while (parser.pos < parser.size && parser.data[parser.pos] != '>') {
                parser.pos++;
            }
            if (parser.pos < parser.size) parser.pos++;
            continue;
        }

        if (strcmp(name, "rect") == 0 || strcmp(name, "circle") == 0 ||
            strcmp(name, "line") == 0 ||
            strcmp(name, "ellipse") == 0) {
            continue;
        }

        while (parser.pos < parser.size && parser.data[parser.pos] != '>') {
            if (parser.pos + 1 < parser.size && parser.data[parser.pos] == '/' &&
                parser.data[parser.pos + 1] == '>') {
                parser.pos += 2;
                break;
            }
            parser.pos++;
        }
        if (parser.pos < parser.size && parser.data[parser.pos] == '>') {
            parser.pos++;
        }
    }

    if (width <= 0 || width > SVG_MAX_WIDTH) width = 256;
    if (height <= 0 || height > SVG_MAX_HEIGHT) height = 256;

    // Calculate viewBox scaling factors
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    int vb_width = 0, vb_height = 0;
    int vb_x = 0, vb_y = 0;

    // Parse viewBox to get scaling factors
    svg_parser_t vb_parser;
    vb_parser.data = (const char*)data;
    vb_parser.size = size;
    vb_parser.pos = 0;

    while (vb_parser.pos < vb_parser.size) {
        svg_parser_skip_whitespace(&vb_parser);
        if (vb_parser.pos >= vb_parser.size || vb_parser.data[vb_parser.pos] != '<') {
            vb_parser.pos++;
            continue;
        }
        vb_parser.pos++;

        if (vb_parser.pos >= vb_parser.size) break;
        if (vb_parser.data[vb_parser.pos] == '/' || vb_parser.data[vb_parser.pos] == '?') {
            // Skip closing tags
            while (vb_parser.pos < vb_parser.size && vb_parser.data[vb_parser.pos] != '>') {
                vb_parser.pos++;
            }
            if (vb_parser.pos < vb_parser.size) vb_parser.pos++;
            continue;
        }

        char vb_name[64];
        svg_parser_read_name(&vb_parser, vb_name, sizeof(vb_name));

        if (strcmp(vb_name, "svg") == 0) {
            char attr_name[64];
            char attr_value[8192];
            while (svg_parser_read_attr(&vb_parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "viewbox") == 0) {
                    float vals[4];
                    int vi = 0;
                    char* saveptr;
                    char* token = strtok_r(attr_value, " ,", &saveptr);
                    while (token != NULL && vi < 4) {
                        vals[vi++] = strtof(token, NULL);
                        token = strtok_r(NULL, " ,", &saveptr);
                    }
                    if (vi >= 4) {
                        vb_x = (int)vals[0];
                        vb_y = (int)vals[1];
                        vb_width = (int)vals[2];
                        vb_height = (int)vals[3];
                        if (vb_width > 0) scale_x = (float)width / vb_width;
                        if (vb_height > 0) scale_y = (float)height / vb_height;
                        SVG_DBG("svg_load_from_memory: viewBox parsed: %f %f %f %f\n", vals[0], vals[1], vals[2], vals[3]);
                    }
                }
            }
            break;
        }

        while (vb_parser.pos < vb_parser.size && vb_parser.data[vb_parser.pos] != '>') {
            vb_parser.pos++;
        }
        if (vb_parser.pos < vb_parser.size) vb_parser.pos++;
    }

    SVG_DBG("svg_load_from_memory: viewBox scaling: scale_x=%f, scale_y=%f, vb=%dx%d, img=%dx%d\n",
            scale_x, scale_y, vb_width, vb_height, width, height);

    svg_image_t* img = (svg_image_t*)malloc(sizeof(svg_image_t));
    if (!img) return NULL;

    img->width = width;
    img->height = height;
    img->pixels = (uint32_t*)malloc((size_t)width * height * sizeof(uint32_t));
    if (!img->pixels) {
        free(img);
        return NULL;
    }

    uint32_t total_pixels = (uint32_t)width * (uint32_t)height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        img->pixels[i] = 0xFFFFFFFF;
    }

    parser.pos = 0;

    uint32_t current_fill = 0xFF000000;
    uint32_t current_stroke = 0xFF000000;
    int current_stroke_width = 0;

    SVG_DBG("svg_load_from_memory: starting second pass, width=%d height=%d\n", width, height);

    while (parser.pos < parser.size) {
        svg_parser_skip_whitespace(&parser);
        svg_parser_skip_comment(&parser);
        svg_parser_skip_whitespace(&parser);

        if (parser.pos >= parser.size) {
            SVG_DBG("svg_load_from_memory: second pass break at end of file, pos %u\n", parser.pos);
            break;
        }
        if (parser.data[parser.pos] != '<') {
            SVG_DBG("svg_load_from_memory: second pass break at pos %u, char='%c' (0x%02x)\n", 
                    parser.pos, parser.data[parser.pos], (unsigned char)parser.data[parser.pos]);
            break;
        }
        parser.pos++;

        if (parser.pos >= parser.size) break;

        if (parser.data[parser.pos] == '/') {
            parser.pos++;
            char name[64];
            svg_parser_read_name(&parser, name, sizeof(name));
            SVG_DBG("svg_load_from_memory: second pass closing tag </%s>\n", name);
            // Skip the closing '>'
            if (parser.pos < parser.size && parser.data[parser.pos] == '>') {
                parser.pos++;
            }
            continue;
        }

        char name[64];
        svg_parser_read_name(&parser, name, sizeof(name));
        SVG_DBG("svg_load_from_memory: second pass tag <%s> at pos %u\n", name, parser.pos);

        if (strcmp(name, "svg") == 0) {
            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
            }

            while (parser.pos < parser.size && parser.data[parser.pos] != '>') {
                parser.pos++;
            }
            if (parser.pos < parser.size) parser.pos++;
            continue;
        }

        if (strcmp(name, "rect") == 0) {
            int x = 0, y = 0, w = width, h = height;
            uint32_t fill = current_fill;
            uint32_t stroke = current_stroke;
            int stroke_width = current_stroke_width;
            svg_transform_t transform;
            transform.trans_x = 0;
            transform.trans_y = 0;
            transform.angle = 0;
            bool has_transform = false;

            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "x") == 0) {
                    x = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "y") == 0) {
                    y = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "width") == 0) {
                    w = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "height") == 0) {
                    h = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "fill") == 0) {
                    fill = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke") == 0) {
                    stroke = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke-width") == 0) {
                    stroke_width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &fill, &stroke, &stroke_width);
                } else if (strcmp(attr_name, "transform") == 0) {
                    svg_parse_transform(attr_value, &transform);
                    has_transform = true;
                }
            }

            if (has_transform) {
                svg_draw_rotated_rect(img->pixels, width, height, x, y, w, h,
                                      transform.trans_x, transform.trans_y, transform.angle,
                                      fill, scale_x, scale_y, vb_x, vb_y);
            } else {
                svg_draw_rect(img->pixels, width, height, x, y, w, h, fill);
            }

            if (stroke_width > 0) {
                svg_draw_line(img->pixels, width, height, x, y, x + w, y, stroke_width, stroke);
                svg_draw_line(img->pixels, width, height, x + w, y, x + w, y + h, stroke_width, stroke);
                svg_draw_line(img->pixels, width, height, x, y + h, x + w, y + h, stroke_width, stroke);
                svg_draw_line(img->pixels, width, height, x, y, x, y + h, stroke_width, stroke);
            }
        } else if (strcmp(name, "circle") == 0) {
            int cx = width / 2, cy = height / 2, r = 10;
            uint32_t fill = current_fill;
            uint32_t stroke = current_stroke;
            int stroke_width = current_stroke_width;

            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "cx") == 0) {
                    cx = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "cy") == 0) {
                    cy = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "r") == 0) {
                    r = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "fill") == 0) {
                    fill = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke") == 0) {
                    stroke = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke-width") == 0) {
                    stroke_width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &fill, &stroke, &stroke_width);
                }
            }

            svg_draw_circle(img->pixels, width, height, cx, cy, r, fill);
        } else if (strcmp(name, "line") == 0) {
            int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            uint32_t stroke = current_stroke;
            int stroke_width = current_stroke_width > 0 ? current_stroke_width : 1;

            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "x1") == 0) {
                    x1 = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "y1") == 0) {
                    y1 = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "x2") == 0) {
                    x2 = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "y2") == 0) {
                    y2 = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "stroke") == 0) {
                    stroke = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke-width") == 0) {
                    stroke_width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &current_fill, &stroke, &stroke_width);
                }
            }

            svg_draw_line(img->pixels, width, height, x1, y1, x2, y2, stroke_width, stroke);
        } else if (strcmp(name, "path") == 0) {
            char* path_data = NULL;
            uint32_t fill = current_fill;
            uint32_t stroke = current_stroke;
            int stroke_width = current_stroke_width;

            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "d") == 0) {
                    path_data = (char*)malloc(strlen(attr_value) + 1);
                    if (path_data) {
                        strcpy(path_data, attr_value);
                    }
                } else if (strcmp(attr_name, "fill") == 0) {
                    fill = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke") == 0) {
                    stroke = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke-width") == 0) {
                    stroke_width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &fill, &stroke, &stroke_width);
                }
            }

            if (path_data) {
                SVG_DBG("svg_load_from_memory: drawing path, fill=0x%08X, stroke=0x%08X, stroke_width=%d\n",
                        fill, stroke, stroke_width);
                svg_draw_path(img->pixels, width, height, path_data, fill, stroke, stroke_width,
                            scale_x, scale_y, vb_x, vb_y);
                free(path_data);
            }
        } else if (strcmp(name, "polygon") == 0) {
            char* points_data = NULL;
            uint32_t fill = current_fill;
            uint32_t stroke = current_stroke;
            int stroke_width = current_stroke_width;

            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "points") == 0) {
                    points_data = (char*)malloc(strlen(attr_value) + 1);
                    if (points_data) {
                        strcpy(points_data, attr_value);
                    }
                } else if (strcmp(attr_name, "fill") == 0) {
                    fill = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke") == 0) {
                    stroke = svg_parse_color(attr_value);
                } else if (strcmp(attr_name, "stroke-width") == 0) {
                    stroke_width = svg_parse_length(attr_value);
                } else if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &fill, &stroke, &stroke_width);
                }
            }

            if (points_data) {
                svg_draw_polygon(img->pixels, width, height, points_data, fill, stroke, stroke_width);
                free(points_data);
            }
        } else if (strcmp(name, "g") == 0 || strcmp(name, "defs") == 0 ||
                   strcmp(name, "title") == 0 || strcmp(name, "desc") == 0 ||
                   strcmp(name, "style") == 0 || strcmp(name, "pattern") == 0 ||
                   strcmp(name, "lineargradient") == 0 || strcmp(name, "radialgradient") == 0) {
            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
                if (strcmp(attr_name, "style") == 0) {
                    svg_parse_style(attr_value, &current_fill, &current_stroke, &current_stroke_width);
                }
            }
        } else {
            char attr_name[64];
            char attr_value[8192];

            while (svg_parser_read_attr(&parser, attr_name, attr_value, sizeof(attr_value))) {
            }
        }

        while (parser.pos < parser.size && parser.data[parser.pos] != '>') {
            if (parser.pos + 1 < parser.size && parser.data[parser.pos] == '/' &&
                parser.data[parser.pos + 1] == '>') {
                parser.pos += 2;
                break;
            }
            parser.pos++;
        }
        if (parser.pos < parser.size && parser.data[parser.pos] == '>') {
            parser.pos++;
        }
    }

    return img;
}

svg_image_t* svg_load(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }

    uint8_t* data = (uint8_t*)malloc(file_size);
    if (!data) {
        fclose(fp);
        return NULL;
    }

    if (fread(data, 1, file_size, fp) != (size_t)file_size) {
        free(data);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    svg_image_t* img = svg_load_from_memory(data, file_size);
    free(data);

    return img;
}

void svg_free(svg_image_t* img) {
    if (img) {
        if (img->pixels) {
            free(img->pixels);
        }
        free(img);
    }
}
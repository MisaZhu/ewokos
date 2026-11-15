#include <Widget/Tree.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

namespace {

class TreeRootNode: public TreeNode {
public:
	void draw(graph_t* g, XTheme* theme, const grect_t& r, bool selected) {
		(void)g; (void)theme; (void)r; (void)selected;
	}
};

}

static void tree_append(std::vector<TreeNode*>& items, TreeNode* node) {
	items.push_back(node);
	if(node->expanded) {
		TreeNode* c = node->firstChild;
		while(c != NULL) {
			tree_append(items, c);
			c = c->next;
		}
	}
}

Tree::Tree() {
	rootNode = new TreeRootNode();
	rootNode->expanded = true;
	rowH = 20;
	indent = 16;
	itemStart = 0;
	pixelOffset = 0;
	selectedNode = NULL;
	setDefaultScrollType(SCROLL_TYPE_V);
}

Tree::~Tree() {
	TreeNode* c = rootNode->firstChild;
	while(c != NULL) {
		TreeNode* nxt = c->next;
		freeNode(c);
		c = nxt;
	}
	delete rootNode;
}

void Tree::freeNode(TreeNode* node) {
	if(node == NULL)
		return;
	TreeNode* c = node->firstChild;
	while(c != NULL) {
		TreeNode* nxt = c->next;
		freeNode(c);
		c = nxt;
	}
	delete node;
}

void Tree::rebuild() {
	items.clear();
	TreeNode* node = rootNode->firstChild;
	while(node != NULL) {
		tree_append(items, node);
		node = node->next;
	}
	revalidateSelection();
}

void Tree::revalidateSelection() {
	if(selectedNode == NULL)
		return;
	for(size_t i = 0; i < items.size(); i++) {
		if(items[i] == selectedNode)
			return;
	}
	selectedNode = NULL;
}

int32_t Tree::selectedIndex() {
	if(selectedNode == NULL)
		return -1;
	for(size_t i = 0; i < items.size(); i++) {
		if(items[i] == selectedNode)
			return (int32_t)i;
	}
	return -1;
}

int32_t Tree::indexAt(int32_t x, int32_t y) {
	(void)x;
	if(rowH == 0)
		return -1;
	int32_t index = (y + pixelOffset) / (int32_t)rowH + itemStart;
	if(index < 0 || index >= (int32_t)items.size())
		return -1;
	return index;
}

void Tree::scrollToIndex(int32_t index) {
	if(rowH == 0)
		return;
	int32_t unit = (int32_t)rowH;
	int32_t viewLen = area.h;
	int32_t pos = itemStart * unit + pixelOffset;
	int32_t top = index * unit;
	int32_t bottom = top + unit;

	if(top < pos)
		pos = top;
	else if(bottom > pos + viewLen)
		pos = bottom - viewLen;

	int32_t maxPos = (int32_t)items.size() * unit - viewLen;
	if(maxPos < 0)
		maxPos = 0;
	if(pos < 0)
		pos = 0;
	if(pos > maxPos)
		pos = maxPos;

	itemStart = pos / unit;
	pixelOffset = pos % unit;
	updateScroller();
}

TreeNode* Tree::addNode(TreeNode* parent, TreeNode* node) {
	if(node == NULL)
		return NULL;
	if(parent == NULL)
		parent = rootNode;

	node->parent = parent;
	node->depth = (parent == rootNode) ? 0 : parent->depth + 1;
	node->next = NULL;
	node->prev = parent->lastChild;
	if(parent->lastChild != NULL)
		parent->lastChild->next = node;
	else
		parent->firstChild = node;
	parent->lastChild = node;

	rebuild();
	updateScroller();
	update();
	return node;
}

void Tree::removeNode(TreeNode* node) {
	if(node == NULL || node == rootNode)
		return;

	TreeNode* parent = node->parent;
	if(node->prev != NULL)
		node->prev->next = node->next;
	else
		parent->firstChild = node->next;
	if(node->next != NULL)
		node->next->prev = node->prev;
	else
		parent->lastChild = node->prev;

	freeNode(node);
	rebuild();
	updateScroller();
	update();
}

void Tree::clear() {
	TreeNode* c = rootNode->firstChild;
	while(c != NULL) {
		TreeNode* nxt = c->next;
		freeNode(c);
		c = nxt;
	}
	rootNode->firstChild = NULL;
	rootNode->lastChild = NULL;
	items.clear();
	selectedNode = NULL;
	itemStart = 0;
	pixelOffset = 0;
	updateScroller();
	update();
}

void Tree::expand(TreeNode* node, bool e) {
	if(node == NULL)
		return;
	if(node->expanded == e)
		return;
	node->expanded = e;
	rebuild();
	updateScroller();
	update();
}

void Tree::collapse(TreeNode* node) {
	expand(node, false);
}

void Tree::toggle(TreeNode* node) {
	if(node == NULL)
		return;
	expand(node, !node->expanded);
}

void Tree::select(TreeNode* node) {
	selectedNode = node;
	if(node != NULL) {
		for(size_t i = 0; i < items.size(); i++) {
			if(items[i] == node) {
				scrollToIndex((int32_t)i);
				break;
			}
		}
	}
	onSelect(node);
	update();
}

void Tree::onSelect(TreeNode* node) {
	(void)node;
}

void Tree::onEnter(TreeNode* node) {
	(void)node;
}

void Tree::setRowH(uint32_t h) {
	if(h == 0)
		return;
	rowH = h;
	onResize();
	update();
}

void Tree::setIndent(uint32_t i) {
	indent = i;
	update();
}

void Tree::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!isAlpha())
		graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	if(rowH == 0 || items.empty())
		return;

	uint32_t num = 0;
	if(items.size() > (size_t)itemStart)
		num = (uint32_t)(items.size() - itemStart);

	uint32_t need = (uint32_t)((r.h + pixelOffset + (int32_t)rowH - 1) / (int32_t)rowH);
	if(num > need)
		num = need;

	for(uint32_t i = 0; i < num; i++) {
		int32_t index = itemStart + (int32_t)i;
		grect_t ir;
		ir.x = r.x;
		ir.y = r.y + (int32_t)(i * rowH) - pixelOffset;
		ir.w = r.w;
		ir.h = (int32_t)rowH;

		grect_t irc = {ir.x, ir.y, ir.w, ir.h};
		grect_insect(&r, &irc);
		if(irc.w <= 0 || irc.h <= 0)
			continue;
		graph_set_clip(g, irc.x, irc.y, irc.w, irc.h);
		drawItem(g, theme, items[index], ir);
	}
}

void Tree::drawItem(graph_t* g, XTheme* theme, TreeNode* node, const grect_t& r) {
	bool selected = (node == selectedNode);
	uint32_t bg = selected ? theme->basic.selectBGColor : theme->basic.bgColor;
	uint32_t fg = selected ? theme->basic.selectColor : theme->basic.fgColor;
	if(!isAlpha() || selected)
		graph_fill_rect(g, r.x, r.y, r.w, r.h, bg);

	int32_t x = r.x + (int32_t)(node->depth * indent);
	int32_t cx = x + (int32_t)(indent / 2);
	int32_t cy = r.y + (int32_t)(rowH / 2);
	int32_t half = (int32_t)(indent / 2) - 2;
	if(half < 2)
		half = 2;

	if(node->hasChildren()) {
		if(node->expanded) {
			graph_fill_triangle(g, cx - half, cy - half,
					cx + half, cy - half,
					cx, cy + half, fg);
		}
		else {
			graph_fill_triangle(g, cx - half, cy - half,
					cx - half, cy + half,
					cx + half, cy, fg);
		}
	}

	grect_t cr;
	cr.x = x + (int32_t)indent + 4;
	cr.y = r.y;
	cr.w = r.x + r.w - cr.x;
	cr.h = r.h;
	node->draw(g, theme, cr, selected);
}

bool Tree::onScroll(int step, bool horizontal) {
	if(horizontal)
		return false;
	int oldStart = itemStart;
	pixelOffset = 0;
	itemStart -= step;

	int32_t rowsInView = rowH > 0 ? (int32_t)(area.h / rowH) : 0;
	int32_t maxStart = (int32_t)items.size() - rowsInView;
	if(maxStart < 0)
		maxStart = 0;
	if(itemStart < 0)
		itemStart = 0;
	if(itemStart > maxStart)
		itemStart = maxStart;

	if(oldStart == itemStart)
		return false;
	updateScroller();
	return true;
}

bool Tree::onScrollPixels(int dx, int dy) {
	(void)dx;
	if(rowH == 0)
		return false;
	if(dx == 0 && dy == 0)
		return false;

	int32_t unit = (int32_t)rowH;
	int32_t viewLen = area.h;
	int32_t contentLen = (int32_t)items.size() * unit;
	int32_t maxPos = contentLen - viewLen;
	if(maxPos < 0)
		maxPos = 0;

	int32_t pos = itemStart * unit + pixelOffset + dy;
	if(pos < 0)
		pos = 0;
	if(pos > maxPos)
		pos = maxPos;

	int32_t newStart = pos / unit;
	int32_t newOff = pos % unit;
	bool changed = (newStart != itemStart) || (newOff != pixelOffset);
	itemStart = newStart;
	pixelOffset = newOff;
	if(changed)
		updateScroller();
	return true;
}

void Tree::updateScroller() {
	if(rowH == 0)
		return;
	uint32_t contentLen = (uint32_t)items.size() * rowH;
	uint32_t pos = (uint32_t)itemStart * rowH + (uint32_t)pixelOffset;
	setScrollerInfo(contentLen, pos, (uint32_t)area.h, false);
}

bool Tree::onMouse(xevent_t* ev) {
	bool ret = Scrollable::onMouse(ev);

	if(ev->state == MOUSE_STATE_DOWN) {
		gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		int32_t index = indexAt(ipos.x, ipos.y);
		if(index >= 0) {
			select(items[index]);
			ret = true;
		}
	}
	else if(ev->state == MOUSE_STATE_CLICK) {
		gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		int32_t index = indexAt(ipos.x, ipos.y);
		if(index >= 0) {
			TreeNode* node = items[index];
			int32_t x = (int32_t)(node->depth * indent);
			if(node->hasChildren() &&
					ipos.x >= x && ipos.x <= x + (int32_t)indent) {
				toggle(node);
			}
			else {
				onEnter(node);
			}
			ret = true;
		}
	}

	if(ret)
		update();
	return ret;
}

bool Tree::onIM(xevent_t* ev) {
	if(ev->state != XIM_STATE_PRESS)
		return false;

	int32_t val = ev->value.im.value;
	int32_t sel = selectedIndex();
	if(val == KEY_UP) {
		if(sel > 0)
			select(items[sel - 1]);
		return true;
	}
	else if(val == KEY_DOWN) {
		if(sel >= 0 && sel < (int32_t)items.size() - 1)
			select(items[sel + 1]);
		return true;
	}
	else if(val == KEY_LEFT) {
		if(selectedNode != NULL) {
			if(selectedNode->expanded && selectedNode->hasChildren())
				collapse(selectedNode);
			else if(selectedNode->parent != NULL && selectedNode->parent != rootNode)
				select(selectedNode->parent);
		}
		return true;
	}
	else if(val == KEY_RIGHT) {
		if(selectedNode != NULL && selectedNode->hasChildren()) {
			if(selectedNode->expanded) {
				if(selectedNode->firstChild != NULL)
					select(selectedNode->firstChild);
			}
			else {
				expand(selectedNode);
			}
		}
		return true;
	}
	else if(val == KEY_ENTER || val == JOYSTICK_START || val == JOYSTICK_A) {
		if(selectedNode != NULL)
			onEnter(selectedNode);
		return true;
	}
	return false;
}

void Tree::onResize() {
	if(rowH > 0 && pixelOffset >= (int32_t)rowH)
		pixelOffset = 0;
	updateScroller();
}

}

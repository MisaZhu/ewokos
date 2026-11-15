#ifndef WIDGET_TREE_HH
#define WIDGET_TREE_HH

#include <Widget/Scrollable.h>
#include <vector>

namespace Ewok {

/* Abstract item: subclasses carry their own data and draw their own content.
 * The tree only manages the hierarchy and the structural chrome (indent,
 * expand/collapse arrow, selection highlight). */
class TreeNode {
public:
	TreeNode* parent;
	TreeNode* firstChild;
	TreeNode* lastChild;
	TreeNode* next;
	TreeNode* prev;
	uint32_t depth;
	bool expanded;

	TreeNode() {
		parent = NULL;
		firstChild = NULL;
		lastChild = NULL;
		next = NULL;
		prev = NULL;
		depth = 0;
		expanded = false;
	}

	virtual ~TreeNode() {}

	bool hasChildren() { return firstChild != NULL; }

	/* draw this item's content into r (indent and arrow space already applied).
	 * selected is true when this node is the current selection. */
	virtual void draw(graph_t* g, XTheme* theme, const grect_t& r, bool selected) = 0;
};

class Tree: public Scrollable {
protected:
	TreeNode* rootNode;
	std::vector<TreeNode*> items;
	uint32_t rowH;
	uint32_t indent;
	int32_t itemStart;
	int32_t pixelOffset;
	TreeNode* selectedNode;

	void rebuild();
	void freeNode(TreeNode* node);
	void revalidateSelection();
	int32_t selectedIndex();
	int32_t indexAt(int32_t x, int32_t y);
	void scrollToIndex(int32_t index);

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	bool onScroll(int step, bool horizontal);
	bool onScrollPixels(int dx, int dy);
	void updateScroller();
	bool onMouse(xevent_t* ev);
	bool onIM(xevent_t* ev);
	void onResize();

	virtual void drawItem(graph_t* g, XTheme* theme, TreeNode* node, const grect_t& r);
	virtual void onSelect(TreeNode* node);
	virtual void onEnter(TreeNode* node);

public:
	Tree();
	~Tree();

	TreeNode* addNode(TreeNode* parent, TreeNode* node);
	void removeNode(TreeNode* node);
	void clear();

	void expand(TreeNode* node, bool e = true);
	void collapse(TreeNode* node);
	void toggle(TreeNode* node);
	void select(TreeNode* node);

	void setRowH(uint32_t h);
	void setIndent(uint32_t i);

	inline uint32_t getRowH() { return rowH; }
	inline TreeNode* getSelected() { return selectedNode; }
	inline TreeNode* getRootNode() { return rootNode; }
};

}

#endif

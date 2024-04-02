#ifndef WIDGET_FILE_WIDGET_HH
#define WIDGET_FILE_WIDGET_HH

#include <Widget/Container.h>

#include <string>
using namespace EwokSTL;
namespace Ewok {

class FileWidget: public Container {
	uint32_t itemSize;
	void build();
protected:
	virtual void onFile(const string& fname, const string& open_with) { }
	virtual void onPath(const string& fname) { }
	virtual void onSelect(const string& fname) { }
public:
	FileWidget();
	inline void load(const string& fname, const string& open_with) { onFile(fname, open_with); }
	inline void enter(const string& pathname) { onPath(pathname); }
	inline void select(const string& fname) { onSelect(fname); }
	inline uint32_t getItemSize() { return itemSize; }
};

}

#endif

#ifndef WIDGET_FILE_WIDGET_HH
#define WIDGET_FILE_WIDGET_HH

#include <Widget/Container.h>
#include <Widget/Grid.h>

#include <string>
using namespace std;
namespace Ewok {

class FileWidget: public Container {
	uint32_t itemSize;
	string path;
	string file;
	Grid*  fileGrid;
	void build();
protected:
	virtual void onFile(const string& fname, const string& open_with) { }
	virtual void onPath(const string& pathName) {  }
	virtual void onSelect(const string& fname) {  }
	void onFocus();

public:
	FileWidget();

	void loadDir(const string& dirName);
	void load(const string& fname, const string& open_with);
	void enter(const string& pathname);
	void select(const string& fname);

	inline uint32_t getItemSize() { return itemSize; }
	inline const string& getPath() {return path;}
	inline const string& getFile() {return file;}
};

}

#endif

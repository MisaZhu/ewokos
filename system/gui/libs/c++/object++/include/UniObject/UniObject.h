#ifndef UNIOBJECT_HH
#define UNIOBJECT_HH

#include <tinyjson/tinyjson.h>

#include <string>
using namespace std;

namespace Ewok {

class UniObject {
protected:
	virtual void setAttr(const string& attr, json_var_t*value) { }
	virtual json_var_t* getAttr(const string& attr) { return NULL; }
	virtual json_var_t* onCall(const string& funName, json_var_t* arg) { return NULL; }
public:
    inline void set(const string& attr, json_var_t* value) { setAttr(attr, value); }
	inline json_var_t* get(const string& attr) { return getAttr(attr); }
	inline json_var_t* call(const string& funcName, json_var_t* arg) { return onCall(funcName, arg); }

    UniObject() {}
    virtual ~UniObject() {}
};

}
#endif

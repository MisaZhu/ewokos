#include <Widget/Stage.h>

namespace Ewok {

Stage::Stage() {
	activeWidget = NULL;
}

void Stage::active(Widget* awd) {
	Widget* wd = children;
	while(wd != NULL) {
		if(wd == awd)
			wd->show();
		else
			wd->hide();
		wd = wd->next;
	}
	update();
}

}
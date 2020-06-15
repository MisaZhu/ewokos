#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <X/X.h>
#include <sys/global.h>

class MyX : public X {
protected:
	void on_repaint() {
	}
public:
	MyX(int x, int y, int w, int h, const char* title, int style):X(x, y, w, h, title, style) {
	}
	~MyX() {
	}
};

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MyX x(10, 10, 220, 200, "gtest", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);
  return 0;
}

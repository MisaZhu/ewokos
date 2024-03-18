var xwin = X.open();

var png = PNG.load("/usr/system/images/mac1984.png");
var font = new Font("/usr/system/fonts/system.ttf", 16);
var x=0, y=0, ix=0, iy=0;

xwin.onRepaint = function(g) {
    g.clear(0xffffffff);
    g.bltAlpha(png, 0, 0, png.width, png.height, ix, iy, png.width, png.height, 0xff);
    g.drawText(10, 50, "Hello, World " + x + ", " + y, font, 0xffff0000);
};

xwin.onMouse = function(mouseEvt) {
    debug(mouseEvt);
    x = mouseEvt.x;
    y = mouseEvt.y;

    if(mouseEvt.state == 3) {
        ix = x;
        iy = y;
    }
    repaint();
};

xwin.onIM = function(imEvt) {
    debug(imEvt);
};

xwin.setVisible(true);
X.run();
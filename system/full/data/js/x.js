var xwin = X.open();

var png = PNG.load("/usr/system/images/mac1984.png");
var font = new Font("/usr/system/fonts/system.ttf", 16);

var mevent = null;
var ix = 0, iy = 0;

xwin.onRepaint = function(g) {
    g.clear(0xffffffff);
    if(mevent == null) {
        g.drawText(0, 0, "try move and click mouse anywhere", font, 0xffff0000);
        return;
    }

    g.bltAlpha(png, 0, 0, png.width, png.height, ix, iy, png.width, png.height, 0xff);
    g.drawText(0, 0, "mouse: " +
            mevent.x + ", " + mevent.y +
            ", state: " + mevent.state, font, 0xffff0000);
};

xwin.onMouse = function(mouseEvt) {
    mevent = mouseEvt;

    if(mouseEvt.state == 3) {
        ix = mouseEvt.x;
        iy = mouseEvt.y;
    }
    repaint();
};

xwin.onIM = function(imEvt) {
    debug(imEvt);
};

xwin.setVisible(true);
X.run();
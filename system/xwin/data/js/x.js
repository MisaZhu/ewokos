var xwin = X.open("js_test", -1, -1, 300, 200);

var png = PNG.load("/data/images/mac.png");
var font = new Font("system");

var mevent = null;
var ievent = null;

var ix = 0, iy = 0;

xwin.onRepaint = function(g) {
    g.clear(0xffffffff);
    g.bltAlpha(png, 0, 0, png.width, png.height, ix, iy, png.width, png.height, 0xff);

    if(mevent == null)
        g.drawText(0, 0, "try move and click mouse anywhere", font, 16, 0xffff0000);
    else 
        g.drawText(0, 0, "mouse: " + 
                mevent.x + ", " +  mevent.y +
                ", state: " + mevent.state,
                font, 16, 0xffff0000);
    
    if(ievent == null)
        g.drawText(0, 20, "try type any key", font, 16, 0xffff0000);
    else 
        g.drawText(0, 20, "key: " + ievent.c +
                ", state: " + ievent.state,
                font, 16, 0xffff0000);
};

xwin.onMouse = function(mouseEvt) {
    mevent = mouseEvt;

    if(mouseEvt.state == 3) {
        ix = mouseEvt.x;
        iy = mouseEvt.y;
    }
    repaint();
};

xwin.onResize = function(size) {
    debug(size);
}

xwin.onIM = function(imEvt) {
    debug(imEvt);
    ievent = imEvt;

    if(imEvt.c == 24) //down
        iy += 8;
    else if(imEvt.c == 5) //up
        iy -= 8;
    else if(imEvt.c == 19) //left
        ix -= 8;
    else if(imEvt.c == 4) //right
        ix += 8;
    repaint();
};

xwin.setVisible(true);
X.run();
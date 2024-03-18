var xwin = X.open();

xwin.i = 0;

xwin.onRepaint = function(g) {
    g.clear(0xffffffff);
    g.fill(10, 10, 40, 40, 0xff000000 | i);
};


xwin.onMouse = function(mouseEvt) {
    debug(mouseEvt);
    i += 0xff;
    repaint();
};

xwin.onIM = function(imEvt) {
    debug(imEvt);
    i += 0xff;
    repaint();
};

xwin.setVisible(true);
X.run();
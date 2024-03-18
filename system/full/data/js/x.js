var xwin = X.open();

xwin.onRepaint = function(g) {
    g.clear(0xffffffff);
    g.fill(10, 10, 40, 40, 0xffff0000);
};

xwin.onMouse = function(mouseEvt) {
    debug(mouseEvt);
};

xwin.onIM = function(imEvt) {
    debug(imEvt);
};

xwin.setVisible(true);
X.run();
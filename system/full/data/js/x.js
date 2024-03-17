var xwin = X.open();

xwin.onRepaint = function(g) {
    debug("js: repaint");
    debug(g);
    g.clear(0xffff0000);
};

xwin.setVisible(true);
X.run();
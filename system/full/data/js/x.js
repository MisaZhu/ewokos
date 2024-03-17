var xwin = X.open();
xwin.onRepaint = function(g) {debug("js: repaint");};
X.run();
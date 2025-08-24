function _onWidgetEvent(widget, event) {
    if(event.type != XEvent.MOUSE || event.state == XEvent.MOUSE_MOVE)
        return;

    var image1 = WJS.getWidgetByName("image1");
    var image2 = WJS.getWidgetByName("image2");

    if(event.state == XEvent.MOUSE_DOWN)
        widget.set("label", "down");
    else if( event.state == XEvent.MOUSE_UP)
        widget.set("label", "up");
    else if( event.state == XEvent.MOUSE_CLICK) {
        if(widget.get("name") == "button1") {
            image1.set("file", "/usr/system/images/logos/apple.png");
            image2.set("file", "/usr/system/images/logos/apple_old.png");
        }
        else if(widget.get("name") == "button2") {
            image1.set("file", "/usr/system/images/logos/apple_old.png");
            image2.set("file", "/usr/system/images/logos/apple.png");
        }
    }
}

function _onMenuItemEvent(menuID) {
    debug("menuID: " + menuID);
}
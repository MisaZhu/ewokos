function _onWidgetEvent(widgetID, widgetName, event) {
    if(event.type != XEvent.MOUSE || event.state != XEvent.MOUSE_CLICK)
        return;
    //debug("widgetID: " + widgetID + ", widgetName: " + widgetName + ", event: " + event);

    var widget = WJS.getWidgetByName(widgetName);
    widget.set("label", "hello world!");
}

function _onMenuItemEvent(menuID) {
    debug("menuID: " + menuID);
}
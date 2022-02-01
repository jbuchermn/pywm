#ifndef _PYWM_WIDGET_H
#define _PYWM_WIDGET_H

struct wm_widget;
struct wm_composite;
struct wm_content;

struct _pywm_widget {
    long handle;
    struct _pywm_widget* next_widget;

    struct wm_widget* widget;
    struct wm_composite* composite;
    struct wm_content* super;
};

void _pywm_widget_init(struct _pywm_widget* _widget, struct wm_widget* widget, struct wm_composite* composite);

void _pywm_widget_update(struct _pywm_widget* widget);

struct _pywm_widgets {
    struct _pywm_widget* first_widget;
};

void _pywm_widgets_init();
long _pywm_widgets_add(struct wm_widget* widget, struct wm_composite* composite);
long _pywm_widgets_get_handle(struct wm_content* content);
long _pywm_widgets_remove(struct wm_content* content);
void _pywm_widgets_update();

struct _pywm_widget* _pywm_widgets_container_from_handle(long handle);
struct wm_content* _pywm_widgets_from_handle(long handle);

#endif

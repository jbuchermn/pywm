#ifndef _PYWM_WIDGET_H
#define _PYWM_WIDGET_H

struct wm_widget;

struct _pywm_widget {
    long handle;
    struct wm_widget* widget;

    struct _pywm_widget* next_widget;
};

void _pywm_widget_init(struct _pywm_widget* _widget, struct wm_widget* widget);

void _pywm_widget_update(struct _pywm_widget* widget);

struct _pywm_widgets {
    struct _pywm_widget* first_widget;
};

void _pywm_widgets_init();
long _pywm_widgets_add(struct wm_widget* widget);
long _pywm_widgets_get_handle(struct wm_widget* widgets);
long _pywm_widgets_remove(struct wm_widget* widget);
void _pywm_widgets_update();

struct _pywm_widget* _pywm_widgets_container_from_handle(long handle);
struct wm_widget* _pywm_widgets_from_handle(long handle);

#endif

#ifndef _PYWM_VIEW_H
#define _PYWM_VIEW_H

struct wm_view;

struct _pywm_view {
    long handle;
    struct wm_view* view;

    struct _pywm_view* next_view;
};

void _pywm_view_init(struct _pywm_view* _view, struct wm_view* view);

void _pywm_view_update(struct _pywm_view* view);

struct _pywm_views {
    struct _pywm_view* first_view;
};

void _pywm_views_init();
long _pywm_views_add(struct wm_view* view);
long _pywm_views_get_handle(struct wm_view* view);
long _pywm_views_remove(struct wm_view* view);
void _pywm_views_update();

#endif

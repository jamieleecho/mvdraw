#include "mvkit/mv_view.h"


bool mv_view_contains_point(const MVView *view, int x, int y) {
    return (x >= view->x) && (x < view->x + view->width) &&
           (y >= view->y) && (y < view->y + view->height);
}


void mv_view_draw(MVView *view) {
    if (view->is_visible && view->draw) {
        view->draw(view);
    }
}


bool mv_view_dispatch_click(MVView *view, MVUiEvent *event) {
    if (!view->is_visible || !view->handle_click) {
        return false;
    }
    return view->handle_click(view, event);
}

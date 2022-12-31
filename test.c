#include "test.h"

#define TAG "TEST"

enum {
    TestSubmenuIndexStart,
    TestSubmenuIndexStop,
    TestSubmenuIndexTest,
};

uint32_t test_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}



void test_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    TestApp* app = context;
    UNUSED(app);

    switch (index) {
        case TestSubmenuIndexStart:
            FURI_LOG_I(TAG, "Start something...");
            break;
        case TestSubmenuIndexStop:
            FURI_LOG_I(TAG, "Stop something...");
            break;
        case TestSubmenuIndexTest:
            FURI_LOG_I(TAG, "Do some test...");
            break;
    }
}


TestApp* test_app_alloc() {
    TestApp* app = malloc(sizeof(TestApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "Start", TestSubmenuIndexStart, test_submenu_callback, app);
    submenu_add_item(app->submenu, "Stop", TestSubmenuIndexStop, test_submenu_callback, app);
    submenu_add_item(app->submenu, "Test", TestSubmenuIndexTest, test_submenu_callback, app);
    view_set_previous_callback(submenu_get_view(app->submenu), test_exit);
    view_dispatcher_add_view(
            app->view_dispatcher, TestViewSubmenu, submenu_get_view(app->submenu));

    app->view_id = TestViewSubmenu;
    view_dispatcher_switch_to_view(app->view_dispatcher, app->view_id);

    return app;
}


void test_app_free(TestApp* app) {
    furi_assert(app);
    view_dispatcher_remove_view(app->view_dispatcher, TestViewSubmenu);
    submenu_free(app->submenu);
    furi_record_close(RECORD_GUI);
    app->gui = NULL;
    free(app);
}


int32_t test_entry(void* p) {
    UNUSED(p);

    TestApp* app = test_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    test_app_free(app);
    return 0;
}

#include "test.h"

#include <inttypes.h>
#include <furi_hal_gpio.h>
#include <furi_hal_light.h>
#include <furi_hal_resources.h>
#include <furi_hal_cortex.h>
#include <stm32wbxx.h>

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

const GpioPin* my_gpio = &gpio_ext_pa4;
uint32_t dur_low, dur_hig;

uint32_t cur_tick, prev_tick = 0;

static void my_isr(void* ctx_) {
    UNUSED(ctx_);

    prev_tick = cur_tick;
    cur_tick = DWT->CYCCNT;

    if (furi_hal_gpio_read(my_gpio)) {
        dur_low = cur_tick - prev_tick;
    } else {
        dur_hig = cur_tick - prev_tick;
    }
}

static void start_interrupts() {
    furi_hal_gpio_init(my_gpio, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_add_int_callback(my_gpio, my_isr, NULL);
    furi_hal_gpio_enable_int_callback(my_gpio);
}

static void stop_interrupts() {
    furi_hal_gpio_disable_int_callback(my_gpio);
    furi_hal_gpio_remove_int_callback(my_gpio);
}

static void do_test() {
    FURI_LOG_I(TAG, "High %"PRIu32, dur_hig / furi_hal_cortex_instructions_per_microsecond());
    FURI_LOG_I(TAG, "Low %"PRIu32, dur_low / furi_hal_cortex_instructions_per_microsecond());
//    FURI_LOG_I(TAG, "Int per us %"PRIu32, furi_hal_cortex_instructions_per_microsecond());
}


void test_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    TestApp* app = context;
    UNUSED(app);

    switch (index) {
        case TestSubmenuIndexStart:
            FURI_LOG_I(TAG, "Start something...");
            furi_hal_light_set(LightRed | LightGreen | LightBlue, 0);
            start_interrupts();
            break;
        case TestSubmenuIndexStop:
            FURI_LOG_I(TAG, "Stop something...");
            stop_interrupts();
            break;
        case TestSubmenuIndexTest:
            FURI_LOG_I(TAG, "Do some test...");
            do_test();
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

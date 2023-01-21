#include "test.h"
#include <furi_hal_cortex.h>
//#include <lib/driver/cc1101_regs.h>

#define TAG "TEST"

// Pinout of external cc1011 module connection to flipper, only g0 has to pb3 due to TIM2_2 special function
const GpioPin* pin_g0 = &gpio_ext_pb3;      // Module 3: TIM2_2 - special pin
const GpioPin* pin_cs = &gpio_ext_pb2;      // Module 4
const GpioPin* pin_ck = &gpio_ext_pa4;      // Module 5
const GpioPin* pin_mosi = &gpio_ext_pa7;    // Module 6
const GpioPin* pin_miso = &gpio_ext_pa6;    // Module 7


enum {
    TestSubmenuIndexStart,
    TestSubmenuIndexStop,
    TestSubmenuIndexTest,
};

uint32_t test_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}


uint8_t sspi_w(uint8_t dat) {
    uint8_t dat_in = 0;

    for (uint8_t mask = 0x80; mask > 0; mask >>= 1) {
        furi_hal_gpio_write(pin_mosi, dat & mask);
        furi_hal_gpio_write(pin_ck, true);
        if (furi_hal_gpio_read(pin_miso))
            dat_in |= mask;
        furi_hal_gpio_write(pin_ck, false);
    }
    return dat_in;
}


void init_cc1011() {
    FURI_LOG_I(TAG, "Reset pins");
    furi_hal_gpio_init(pin_g0, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_cs, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_ck, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_mosi, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_miso, GpioModeInput, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_write(pin_cs, true);
    furi_hal_gpio_write(pin_cs, false);

    while(furi_hal_gpio_read(pin_miso))
        ;
    FURI_LOG_I(TAG, "Chip is ready");
    uint8_t res = sspi_w(0x30);
    FURI_LOG_I(TAG, "Reset, out=%x", res);
}


void deinit_cc1101() {
    FURI_LOG_I(TAG, "Deinit cc1101");
    furi_hal_gpio_write(pin_cs, true);

    furi_hal_gpio_init(pin_g0, GpioModeInput, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_cs, GpioModeInput, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_ck, GpioModeInput, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_mosi, GpioModeInput, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(pin_miso, GpioModeInput, GpioPullNo, GpioSpeedLow);
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
            init_cc1011();
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

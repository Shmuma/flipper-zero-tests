#include "test.h"

#include <inttypes.h>
#include <furi_hal_gpio.h>
#include <furi_hal_light.h>
#include <furi_hal_resources.h>
#include <furi_hal_cortex.h>
#include <stm32wbxx.h>
#include <stm32wbxx_ll_tim.h>
#include <furi_hal_interrupt.h>

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

const GpioPin* my_gpio = &gpio_ext_pb3;
volatile uint32_t dur, dur_low, dur_hig;

//volatile uint64_t prev_dwt = 0;
//volatile uint64_t last_dwt = 0;
//volatile uint64_t dur;
//
//static void my_isr(void* ctx_) {
//    UNUSED(ctx_);
//
//    prev_dwt = last_dwt;
//    last_dwt += DWT->CYCCNT - (uint32_t)(last_dwt);
//    dur = last_dwt - prev_dwt;
//
//    if (furi_hal_gpio_read(my_gpio)) {
//        dur_low = dur;
//    } else {
//        dur_hig = dur;
//    }
//}


static void my_isr_tim(void* ctx_) {
    UNUSED(ctx_);

    // Channel 1
    if(LL_TIM_IsActiveFlag_CC1(TIM2)) {
        LL_TIM_ClearFlag_CC1(TIM2);
        dur = LL_TIM_IC_GetCaptureCH1(TIM2);
        dur_hig = dur;
    }
    // Channel 2
    if(LL_TIM_IsActiveFlag_CC2(TIM2)) {
        LL_TIM_ClearFlag_CC2(TIM2);
        dur_low = LL_TIM_IC_GetCaptureCH2(TIM2) - dur;
    }
}


static void start_interrupts() {
//    furi_hal_gpio_init(my_gpio, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedHigh);
//    furi_hal_gpio_add_int_callback(my_gpio, my_isr, NULL);
//    furi_hal_gpio_enable_int_callback(my_gpio);
    furi_hal_gpio_init_ex(my_gpio, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedLow, GpioAltFn1TIM2);

    // Timer: base
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.Prescaler = 64 - 1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 0x7FFFFFFE;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV4;
    LL_TIM_Init(TIM2, &TIM_InitStruct);

    // Timer: advanced
    LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_DisableARRPreload(TIM2);
    LL_TIM_SetTriggerInput(TIM2, LL_TIM_TS_TI2FP2);
    LL_TIM_SetSlaveMode(TIM2, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
    LL_TIM_EnableMasterSlaveMode(TIM2);
    LL_TIM_DisableDMAReq_TRIG(TIM2);
    LL_TIM_DisableIT_TRIG(TIM2);

    // Timer: channel 1 indirect
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_INDIRECTTI);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_FALLING);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);

    // Timer: channel 2 direct
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV32_N8);

    // ISR setup
    furi_hal_interrupt_set_isr(FuriHalInterruptIdTIM2, my_isr_tim, NULL);

    // Interrupts and channels
    LL_TIM_EnableIT_CC1(TIM2);
    LL_TIM_EnableIT_CC2(TIM2);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);

    // Start timer
    LL_TIM_SetCounter(TIM2, 0);
    LL_TIM_EnableCounter(TIM2);
}

static void stop_interrupts() {
    FURI_CRITICAL_ENTER();
    LL_TIM_DeInit(TIM2);
    FURI_CRITICAL_EXIT();
    furi_hal_interrupt_set_isr(FuriHalInterruptIdTIM2, NULL, NULL);
    furi_hal_gpio_init(my_gpio, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
//
//    furi_hal_gpio_disable_int_callback(my_gpio);
//    furi_hal_gpio_remove_int_callback(my_gpio);
}

static void do_test() {
    FURI_LOG_I(TAG, "High %"PRIu32, dur_hig >> 6);
    FURI_LOG_I(TAG, "Low %"PRIu32, dur_low >> 6);
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

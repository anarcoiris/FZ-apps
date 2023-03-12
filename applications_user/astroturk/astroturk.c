#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <furi_hal.h>
#include <furi_hal_resources.h>
#include <furi_hal_gpio.h>

#define TAG "Astroturk"
bool Milisec=false;
bool status_drv = true;
bool status_dir = false;
bool status_step = false;
typedef enum {
    EventTypeKey,
    EventTypeTick,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} AstroturkEvent;

typedef struct {
    int TimeExp;    // this is the exposition time to set
    int NumbExp;    //this is the number of expositions to shoot
    int TimeExpMS;  //this is exposition time converted to milisecond

} AstroturkState;

static void astroturk_render_callback(Canvas* const canvas, void* ctx) {
    const AstroturkState* astroturk_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(astroturk_state == NULL) {
        return;
    }
    // border around the edge of the screen
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, astroturk_state->TimeExp, astroturk_state->NumbExp, AlignRight, AlignBottom, ".");
    elements_multiline_text_aligned(
        canvas, 96, 2, AlignCenter, AlignTop, "Astroturk by Santi");
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(
        canvas, 32, 24, AlignCenter, AlignTop, " < Exp Number >");
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%u", astroturk_state->NumbExp);
    elements_multiline_text_aligned(
        canvas, 70, 24, AlignCenter, AlignTop, buffer);
    elements_multiline_text_aligned(
        canvas, 32, 46, AlignCenter, AlignTop, "^ Time v");
    snprintf(buffer, sizeof(buffer), "%u", astroturk_state->TimeExp);
    elements_multiline_text_aligned(
        canvas, 70, 46, AlignCenter, AlignTop, buffer);
    if(Milisec==true){
        elements_multiline_text_aligned(
            canvas, 86, 46, AlignCenter, AlignTop, "ms");
    }
    if(status_drv==true){
        elements_multiline_text_aligned(
            canvas, 92, 28, AlignCenter, AlignTop, "drv on");
    }
    if(status_drv==false){
        elements_multiline_text_aligned(
            canvas, 92, 28, AlignCenter, AlignTop, "drv off");
    }
    if(status_step==true){
        elements_multiline_text_aligned(
            canvas, 92, 36, AlignCenter, AlignTop, "step on");
    }
    if(status_step==false){
        elements_multiline_text_aligned(
            canvas, 92, 36, AlignCenter, AlignTop, "step off");
    }
    if(status_dir==true){
        elements_multiline_text_aligned(
            canvas, 92, 44, AlignCenter, AlignTop, "dir 1");
    }
    if(status_dir==false){
        elements_multiline_text_aligned(
            canvas, 92, 44, AlignCenter, AlignTop, "dir 0");
    }
    release_mutex((ValueMutex*)ctx, astroturk_state);
}

static void astroturk_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    AstroturkEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void astroturk_state_init(AstroturkState* const astroturk_state) {
    astroturk_state->NumbExp = 20;
    astroturk_state->TimeExp = 30;
}

int32_t astroturk_app(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(AstroturkEvent));

    AstroturkState* astroturk_state = malloc(sizeof(AstroturkState));
    astroturk_state_init(astroturk_state);

    ValueMutex state_mutex;
    if (!init_mutex(&state_mutex, astroturk_state, sizeof(AstroturkState))) {
        FURI_LOG_E("astroturk", "cannot create mutex\r\n");
        free(astroturk_state);
        return 255;
    }

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, astroturk_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, astroturk_input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    //Enabled stepper motor pin
    const GpioPin gpio_drv = (GpioPin){.port = GPIOA, .pin = LL_GPIO_PIN_14};
    const GpioPin gpio_step = (GpioPin){.port = GPIOA, .pin = LL_GPIO_PIN_13};
    const GpioPin gpio_dir = (GpioPin){.port = USART1_TX_Port, .pin = USART1_TX_Pin};    //{.port = GPIOA, .pin = LL_GPIO_PIN_6}; this once was previous setting, nw
    furi_hal_gpio_init_simple(&gpio_drv, GpioModeOutputPushPull);  //trigger the a7 On for as long as TimeExp time is.
    furi_hal_gpio_write(&gpio_drv,true);
    furi_hal_gpio_init_simple(&gpio_dir, GpioModeOutputPushPull);
    furi_hal_gpio_write(&gpio_dir,false);
    furi_hal_gpio_init_simple(&gpio_step, GpioModeOutputPushPull);
    furi_hal_gpio_write(&gpio_step,false);

    AstroturkEvent event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        AstroturkState* astroturk_state = (AstroturkState*)acquire_mutex_block(&state_mutex);

        if(event_status == FuriStatusOk) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                    case InputKeyUp:
                        if(astroturk_state->TimeExp>9){
                            if(astroturk_state->TimeExp>44){
                                astroturk_state->TimeExp = astroturk_state->TimeExp+15;
                            }else{
                                astroturk_state->TimeExp = astroturk_state->TimeExp+5;
                                }
                        }if(astroturk_state->TimeExp<=9){
                            astroturk_state->TimeExp++;
                        }
                        break;
                    case InputKeyDown:
                        if(astroturk_state->TimeExp<=5){
                          if(astroturk_state->TimeExp>1){
                            astroturk_state->TimeExp--;
                          }
                        }if(astroturk_state->TimeExp>5){
                          if(astroturk_state->TimeExp>44){
                            astroturk_state->TimeExp = astroturk_state->TimeExp-15;
                          }else{
                            astroturk_state->TimeExp = astroturk_state->TimeExp-5;
                          }
                        }
                        break;
                    case InputKeyRight:
                        if(astroturk_state->NumbExp>9){
                            if(astroturk_state->NumbExp>44){
                              astroturk_state->NumbExp = astroturk_state->NumbExp+15;
                            }else{
                              astroturk_state->NumbExp = astroturk_state->NumbExp+5;
                              }
                            }
                        if(astroturk_state->NumbExp<=9){
                                  astroturk_state->NumbExp++;
                        }
                        break;
                    case InputKeyLeft:
                      if(astroturk_state->NumbExp<=5){
                        if(astroturk_state->NumbExp>1){
                          astroturk_state->NumbExp--;
                        }
                      }if(astroturk_state->NumbExp>5){
                        if(astroturk_state->NumbExp>44){
                          astroturk_state->NumbExp = astroturk_state->NumbExp-15;
                        }else{
                          astroturk_state->NumbExp = astroturk_state->NumbExp-5;
                        }
                      }
                    break;
                    case InputKeyOk:
                      if(status_drv==true){
                        furi_hal_gpio_write(&gpio_drv,false);
                        status_drv=!status_drv;
                        break;
                      }
                      if(status_drv==false){
                        furi_hal_gpio_write(&gpio_drv,true);
                        status_drv=!status_drv;
                        break;
                      }
                        break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    default:
                        break;
                    }
                }
                if(event.input.type == InputTypeLong) {
                    switch(event.input.key) {
                    case InputKeyDown:
                        Milisec=!Milisec;
                        furi_hal_gpio_write(&gpio_drv,false);
                        status_drv=false;
                    break;
                    case InputKeyRight:
                      if(status_step==false){
                        furi_hal_gpio_write(&gpio_step,true);
                        status_step=!status_step;
                        break;
                      }
                      if(status_step==true){
                        furi_hal_gpio_write(&gpio_step,false);
                        status_step=!status_step;
                        break;
                      }
                    break;
                    case InputKeyLeft:
                      if(status_dir==false){
                        furi_hal_gpio_write(&gpio_dir,true);
                        status_dir=!status_dir;
                        break;
                      }
                      if(status_dir==true){
                        furi_hal_gpio_write(&gpio_dir,false);
                        status_dir=!status_dir;
                        break;
                      }
                    break;
                    case InputKeyOk:
                      if(Milisec==false){
                        astroturk_state->TimeExpMS = astroturk_state->TimeExp*1000;
                      }else{
                        astroturk_state->TimeExpMS = astroturk_state->TimeExp;
                      }
                      for(int i = 0; i < astroturk_state->NumbExp; i++){ //So basically this is the loop that will;
                                                                //  next ver will use 'astroturk_state->NumbExp--'
                                                                //if we can display the NumbExp countdown somehow while for loop runs

                        //acquire_mutex_block(&state_mutex); //AstroturkState* astroturk_state = (AstroturkState*)   ## place in front!!
                        furi_hal_gpio_init_simple(&gpio_ext_pa7, GpioModeOutputPushPull);  //trigger the a7 On for as long as TimeExp time is.
                        //furi_hal_gpio_init_simple(&gpio_ext_pc3, GpioModeOutputPushPull);  //trigger the a7 On for as long as TimeExp time is.
                        furi_hal_gpio_write(&gpio_ext_pa7,true);
                        furi_hal_gpio_write(&gpio_step,true);
                        //furi_hal_gpio_write(&gpio_ext_pc3,true);
                        furi_delay_ms(astroturk_state->TimeExpMS);
                        furi_delay_ms(1000);
                        furi_hal_gpio_write(&gpio_ext_pa7,false);
                        furi_hal_gpio_write(&gpio_step,false);
                        //furi_hal_gpio_write(&gpio_ext_pc3,false);
                        furi_delay_ms(500);
                      //  view_port_update(view_port);        ## testing to get progress status screen
                        //release_mutex(&state_mutex, astroturk_state);
                        //view_port_update(view_port);
                        //furi_delay_ms(500);
                        }
                      //furi_hal_gpio_write(&gpio_ext_pa14,false);
                      //furi_hal_gpio_disable_int_callback(&gpio_ext_pa14);
                      furi_hal_gpio_disable_int_callback(&gpio_ext_pa7);  // Then OFF the pin
                      //furi_hal_gpio_disable_int_callback(&gpio_ext_pc3);
                    break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    default:
                        break;
                      }
                    }
                  }
                }else {
            // event timeout on test
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, astroturk_state);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);


    return 0;
}

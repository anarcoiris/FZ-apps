#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <furi_hal.h>
#include <furi_hal_resources.h>
#include <furi_hal_gpio.h>

#define TAG "Intervalometro"
bool Milisec=false;
typedef enum {
    EventTypeKey,
    EventTypeTick,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} iMetroEvent;

typedef struct {
    int TimeExp;    // this is the exposition time to set
    int NumbExp;    //this is the number of expositions to shoot
    int TimeExpMS;  //this is exposition time converted to milisecond

} iMetroState;

static void imetro_render_callback(Canvas* const canvas, void* ctx) {
    const iMetroState* imetro_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(imetro_state == NULL) {
        return;
    }
    // border around the edge of the screen
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, imetro_state->TimeExp, imetro_state->NumbExp, AlignRight, AlignBottom, ".");
    elements_multiline_text_aligned(
        canvas, 88, 2, AlignCenter, AlignTop, "Intervalometro by Santi");
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(
        canvas, 32, 24, AlignCenter, AlignTop, " < Exp Number >");
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%u", imetro_state->NumbExp);
    elements_multiline_text_aligned(
        canvas, 70, 24, AlignCenter, AlignTop, buffer);
    elements_multiline_text_aligned(
        canvas, 32, 46, AlignCenter, AlignTop, "^ Time v");
    snprintf(buffer, sizeof(buffer), "%u", imetro_state->TimeExp);
    elements_multiline_text_aligned(
        canvas, 70, 46, AlignCenter, AlignTop, buffer);
    if(Milisec==true){
        elements_multiline_text_aligned(
            canvas, 86, 46, AlignCenter, AlignTop, "ms");
    }
    release_mutex((ValueMutex*)ctx, imetro_state);
}

static void imetro_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    iMetroEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void imetro_state_init(iMetroState* const imetro_state) {
    imetro_state->NumbExp = 20;
    imetro_state->TimeExp = 30;
}

int32_t imetro_app(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(iMetroEvent));

    iMetroState* imetro_state = malloc(sizeof(iMetroState));
    imetro_state_init(imetro_state);

    ValueMutex state_mutex;
    if (!init_mutex(&state_mutex, imetro_state, sizeof(iMetroState))) {
        FURI_LOG_E("imetro", "cannot create mutex\r\n");
        free(imetro_state);
        return 255;
    }

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, imetro_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, imetro_input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    iMetroEvent event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        iMetroState* imetro_state = (iMetroState*)acquire_mutex_block(&state_mutex);

        if(event_status == FuriStatusOk) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                    case InputKeyUp:
                        if(imetro_state->TimeExp>9){
                            if(imetro_state->TimeExp>44){
                                imetro_state->TimeExp = imetro_state->TimeExp+15;
                            }else{
                                imetro_state->TimeExp = imetro_state->TimeExp+5;
                                }
                        }if(imetro_state->TimeExp<=9){
                            imetro_state->TimeExp++;
                        }
                        break;
                    case InputKeyDown:
                        if(imetro_state->TimeExp<=5){
                          if(imetro_state->TimeExp>1){
                            imetro_state->TimeExp--;
                          }
                        }if(imetro_state->TimeExp>5){
                          if(imetro_state->TimeExp>44){
                            imetro_state->TimeExp = imetro_state->TimeExp-15;
                          }else{
                            imetro_state->TimeExp = imetro_state->TimeExp-5;
                          }
                        }
                        break;
                    case InputKeyRight:
                        if(imetro_state->NumbExp>9){
                            if(imetro_state->NumbExp>44){
                              imetro_state->NumbExp = imetro_state->NumbExp+15;
                            }else{
                              imetro_state->NumbExp = imetro_state->NumbExp+5;
                              }
                            }
                        if(imetro_state->NumbExp<=9){
                                  imetro_state->NumbExp++;
                        }
                        break;
                    case InputKeyLeft:
                      if(imetro_state->NumbExp<=5){
                        if(imetro_state->NumbExp>1){
                          imetro_state->NumbExp--;
                        }
                      }if(imetro_state->NumbExp>5){
                        if(imetro_state->NumbExp>44){
                          imetro_state->NumbExp = imetro_state->NumbExp-15;
                        }else{
                          imetro_state->NumbExp = imetro_state->NumbExp-5;
                        }
                      }
                    break;
                    case InputKeyOk:
                        if(Milisec==false){
                          imetro_state->TimeExpMS = imetro_state->TimeExp*1000;
                        }else{
                          imetro_state->TimeExpMS = imetro_state->TimeExp;
                        }
                        for(int i = 0; i < imetro_state->NumbExp; i++){ //So basically this is the loop that will;
                                                                  //  next ver will use 'imetro_state->NumbExp--'
                                                                  //if we can display the NumbExp countdown somehow while for loop runs

                          //acquire_mutex_block(&state_mutex); //iMetroState* imetro_state = (iMetroState*)   ## place in front!!
                          furi_hal_gpio_init_simple(&gpio_ext_pa7, GpioModeOutputPushPull);  //trigger the a7 On for as long as TimeExp time is.
                          furi_hal_gpio_write(&gpio_ext_pa7,true);
			  furi_delay_ms(imetro_state->TimeExpMS);
			  furi_delay_ms(1000);
			  furi_hal_gpio_write(&gpio_ext_pa7,false);
                          furi_delay_ms(500);
                        //  view_port_update(view_port);        ## testing to get progress status screen
                          //release_mutex(&state_mutex, imetro_state);
                          //view_port_update(view_port);
                          //furi_delay_ms(500);
                          }
                        furi_hal_gpio_disable_int_callback(&gpio_ext_pa7);  // Then OFF the pin
                        break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    default:
                        break;
                    }
                }
              if(event.input.type == InputTypeLong && event.input.key == InputKeyDown){
                Milisec=!Milisec;
              }
            }
        } else {
            // event timeout on test
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, imetro_state);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);


    return 0;
}

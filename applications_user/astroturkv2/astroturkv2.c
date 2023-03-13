#include <furi.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <input/input.h>
#include <stdlib.h>
#include <furi_hal.h>
#include <furi_hal_resources.h>
#include <furi_hal_gpio.h>

#define TAG "Astroturkv2"
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
} Astroturkv2Event;

typedef struct {
    int TimeExp;    // this is the exposition time to set
    int NumbExp;    //this is the number of expositions to shoot
    int TimeExpMS;  //this is exposition time converted to milisecond
    int MenuItem;  //this is the menu item selector
    int Steps;

} Astroturkv2State;

static void astroturkv2_render_callback(Canvas* const canvas, void* ctx) {
    const Astroturkv2State* astroturkv2_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(astroturkv2_state == NULL) {
        return;
    }
    // border around the edge of the screen
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 6);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, astroturkv2_state->TimeExp, astroturkv2_state->NumbExp, AlignRight, AlignBottom, ".");
    elements_multiline_text_aligned(
        canvas, 90, 2, AlignCenter, AlignTop, "Astroturk v.2 by Santi");
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(
        canvas, 32, 24, AlignCenter, AlignTop, " -Settings-");
    char buffer[12];
    //snprintf(buffer, sizeof(buffer), "%u", astroturkv2_state->NumbExp);
    //elements_multiline_text_aligned(
    //    canvas, 70, 24, AlignCenter, AlignTop, buffer);
    switch(astroturkv2_state->MenuItem)  {
      case 1:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, "< Time >");
        snprintf(buffer, sizeof(buffer), "%u", astroturkv2_state->TimeExp);
        if(Milisec==true){
            elements_multiline_text_aligned(
                canvas, 112, 46, AlignCenter, AlignTop, "ms");
        }
      break;
      case 2:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, " < Exp Number >");
        snprintf(buffer, sizeof(buffer), "%u", astroturkv2_state->NumbExp);
      break;
      case 3:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, " < Driver >");
        if(status_drv==true){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "ON!");
        }
        if(status_drv==false){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "OFF!");
        }
      break;
      case 4:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, " < Steps >");
        snprintf(buffer, sizeof(buffer), "%u", astroturkv2_state->Steps);
      break;
      case 5:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, " < Direccion >");
        if(status_dir==true){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "1");
        }
        if(status_dir==false){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "0");
        }
      break;
      case 6:
        elements_multiline_text_aligned(
            canvas, 32, 46, AlignCenter, AlignTop, " < Step status >");
        if(status_step==true){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "ON!");
        }
        if(status_step==false){
            elements_multiline_text_aligned(
              canvas, 96, 46, AlignCenter, AlignTop, "OFF!");
        }
      break;
        }
    elements_multiline_text_aligned(
        canvas, 96, 46, AlignCenter, AlignTop, buffer);
    release_mutex((ValueMutex*)ctx, astroturkv2_state);
}

static void astroturkv2_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    Astroturkv2Event event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void astroturkv2_state_init(Astroturkv2State* const astroturkv2_state) {
    astroturkv2_state->NumbExp = 20;
    astroturkv2_state->TimeExp = 30;
    astroturkv2_state->MenuItem = 1;
    astroturkv2_state->Steps = 200;
}

int32_t astroturkv2_app(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(Astroturkv2Event));

    Astroturkv2State* astroturkv2_state = malloc(sizeof(Astroturkv2State));
    astroturkv2_state_init(astroturkv2_state);

    ValueMutex state_mutex;
    if (!init_mutex(&state_mutex, astroturkv2_state, sizeof(Astroturkv2State))) {
        FURI_LOG_E("astroturkv2", "cannot create mutex\r\n");
        free(astroturkv2_state);
        return 255;
    }

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, astroturkv2_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, astroturkv2_input_callback, event_queue);

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
    furi_hal_gpio_init(&gpio_step, GpioModeOutputPushPull, GpioPullUp, GpioSpeedVeryHigh);
    //furi_hal_gpio_init_simple(&gpio_step,GpioModeOutputOpenDrain);
    furi_hal_gpio_write(&gpio_step,false);

    Astroturkv2Event event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        Astroturkv2State* astroturkv2_state = (Astroturkv2State*)acquire_mutex_block(&state_mutex);

        if(event_status == FuriStatusOk) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                    case InputKeyUp:
                        astroturkv2_state->MenuItem--;
                        if(astroturkv2_state->MenuItem<1){
                          astroturkv2_state->MenuItem=6;
                          break;
                        }
                        break;                                    //if(astroturkv2_state->TimeExp>9){
                                                                //if(astroturkv2_state->TimeExp>44){
                                                                    //astroturkv2_state->TimeExp = astroturkv2_state->TimeExp+15;
                                                                  //}else{
                                                                    //astroturkv2_state->TimeExp = astroturkv2_state->TimeExp+5;
                                                                    //}
                                                                  //}if(astroturkv2_state->TimeExp<=9){
                                                                    //astroturkv2_state->TimeExp++;
                                                                  //}
                                                                  //break;
                    case InputKeyDown:
                      astroturkv2_state->MenuItem++;
                      if(astroturkv2_state->MenuItem>6){
                        astroturkv2_state->MenuItem=1;
                        break;
                      }
                    break;
                        //if(astroturkv2_state->TimeExp<=5){
                        //  if(astroturkv2_state->TimeExp>1){
                        //    astroturkv2_state->TimeExp--;
                        //  }
                        //}if(astroturkv2_state->TimeExp>5){
                        //  if(astroturkv2_state->TimeExp>44){
                        //    astroturkv2_state->TimeExp = astroturkv2_state->TimeExp-15;
                        //  }else{
                        //    astroturkv2_state->TimeExp = astroturkv2_state->TimeExp-5;
                        //  }
                        //}
                        //break;
                    case InputKeyRight:
                        if(astroturkv2_state->MenuItem==1){
                          if(astroturkv2_state->TimeExp>9){
                            if(astroturkv2_state->TimeExp>44){
                              astroturkv2_state->TimeExp = astroturkv2_state->TimeExp+15;
                            }else{
                              astroturkv2_state->TimeExp = astroturkv2_state->TimeExp+5;
                              }
                          }if(astroturkv2_state->TimeExp<=9){
                              astroturkv2_state->TimeExp++;
                            }
                          }
                        if(astroturkv2_state->MenuItem==2){
                          if(astroturkv2_state->NumbExp>9){
                              if(astroturkv2_state->NumbExp>44){
                                astroturkv2_state->NumbExp = astroturkv2_state->NumbExp+15;
                              }else{
                                astroturkv2_state->NumbExp = astroturkv2_state->NumbExp+5;
                                }
                              }
                          if(astroturkv2_state->NumbExp<=9){
                                    astroturkv2_state->NumbExp++;
                          }
                        }
                        if(astroturkv2_state->MenuItem==3){
                          if(status_drv==false){
                            furi_hal_gpio_write(&gpio_drv,true);
                            status_drv=!status_drv;
                            break;
                          }
                          if(status_drv==true){
                            furi_hal_gpio_write(&gpio_drv,false);
                            status_drv=!status_drv;
                            break;
                          }
                        }
                        if(astroturkv2_state->MenuItem==4){
                          if(astroturkv2_state->Steps>9){
                              if(astroturkv2_state->Steps>44){
                                astroturkv2_state->Steps = astroturkv2_state->Steps+15;
                              }else{
                                astroturkv2_state->Steps = astroturkv2_state->Steps+5;
                                }
                              }
                          if(astroturkv2_state->Steps<=9){
                                    astroturkv2_state->Steps++;
                          }
                        }
                        if(astroturkv2_state->MenuItem==5){
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
                        }
                        if(astroturkv2_state->MenuItem==6){
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
                        }
                    break;
                    case InputKeyLeft:
                      if(astroturkv2_state->MenuItem==1){
                        if(astroturkv2_state->TimeExp<=5){
                          if(astroturkv2_state->TimeExp>1){
                            astroturkv2_state->TimeExp--;
                          }
                        }if(astroturkv2_state->TimeExp>5){
                          if(astroturkv2_state->TimeExp>44){
                            astroturkv2_state->TimeExp = astroturkv2_state->TimeExp-15;
                          }else{
                            astroturkv2_state->TimeExp = astroturkv2_state->TimeExp-5;
                          }
                        }
                      }
                      if(astroturkv2_state->MenuItem==2){
                        if(astroturkv2_state->NumbExp<=5){
                          if(astroturkv2_state->NumbExp>1){
                            astroturkv2_state->NumbExp--;
                          }
                        }if(astroturkv2_state->NumbExp>5){
                          if(astroturkv2_state->NumbExp>44){
                            astroturkv2_state->NumbExp = astroturkv2_state->NumbExp-15;
                          }else{
                            astroturkv2_state->NumbExp = astroturkv2_state->NumbExp-5;
                          }
                        }
                      }
                      if(astroturkv2_state->MenuItem==3){
                        if(status_drv==false){
                          furi_hal_gpio_write(&gpio_drv,true);
                          status_drv=!status_drv;
                          break;
                        }
                        if(status_drv==true){
                          furi_hal_gpio_write(&gpio_drv,false);
                          status_drv=!status_drv;
                          break;
                        }
                      }
                      if(astroturkv2_state->MenuItem==4){
                        if(astroturkv2_state->Steps<=5){
                          if(astroturkv2_state->Steps>1){
                            astroturkv2_state->Steps--;
                          }
                        }if(astroturkv2_state->Steps>5){
                          if(astroturkv2_state->Steps>44){
                            astroturkv2_state->Steps = astroturkv2_state->Steps-15;
                          }else{
                            astroturkv2_state->Steps = astroturkv2_state->Steps-5;
                          }
                        }
                      }
                      if(astroturkv2_state->MenuItem==5){
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
                      }
                      if(astroturkv2_state->MenuItem==6){
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
                      }
                      break;
                    case InputKeyOk:
                      furi_hal_gpio_write(&gpio_drv,true);
                      if(Milisec==false){
                        astroturkv2_state->TimeExpMS = astroturkv2_state->TimeExp*1000;
                      }else{
                        astroturkv2_state->TimeExpMS = astroturkv2_state->TimeExp;
                      }
                      astroturkv2_state->Steps = (astroturkv2_state->TimeExpMS+50)/3;
                      for(int i = 0; i < astroturkv2_state->NumbExp; i++){ //So basically this is the loop that will;
                                                          //  next ver will use 'astroturkv2_state->NumbExp--'
                                                          //if we can display the NumbExp countdown somehow while for loop runs

                                                                //acquire_mutex_block(&state_mutex); //Astroturkv2State* astroturkv2_state = (Astroturkv2State*)   ## place in front!!
                              furi_hal_gpio_init(&gpio_ext_pa7, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);  //trigger the a7 On for as long as TimeExp time is.
                                                                //furi_hal_gpio_init_simple(&gpio_ext_pc3, GpioModeOutputPushPull);  //trigger the a7 On for as long as TimeExp time is.
                              furi_hal_gpio_write(&gpio_ext_pa7,true);
                              for(int s = 0; s < astroturkv2_state->Steps; s++){
                                  furi_hal_gpio_write(&gpio_step,true);
                                  furi_delay_us(2000);
                                  furi_hal_gpio_write(&gpio_step,false);
                                  furi_delay_us(1000);
                              }

                                                                //furi_hal_gpio_write(&gpio_ext_pc3,true);
                              furi_hal_gpio_write(&gpio_ext_pa7,false);
                              //furi_hal_gpio_write(&gpio_step,false);
                                                                //furi_hal_gpio_write(&gpio_ext_pc3,false);
                              furi_delay_us(1000);
                                                                //  view_port_update(view_port);        ## testing to get progress status screen
                                                                //release_mutex(&state_mutex, astroturkv2_state);
                                                                //view_port_update(view_port);
                                                                //furi_delay_ms(500);
                      }
                                                              //furi_hal_gpio_write(&gpio_ext_pa14,false);
                                                              //furi_hal_gpio_disable_int_callback(&gpio_ext_pa14);
                      furi_hal_gpio_disable_int_callback(&gpio_ext_pa7);  // Then OFF the pin
                                                              //furi_hal_gpio_disable_int_callback(&gpio_ext_pc3);
                      break;

                      //if(status_drv==true){
                      //  furi_hal_gpio_write(&gpio_drv,false);
                      //  status_drv=!status_drv;
                      //  break;
                      //}
                      //if(status_drv==false){
                      //  furi_hal_gpio_write(&gpio_drv,true);
                      //  status_drv=!status_drv;
                      //  break;
                      //}
                      //break;
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
        release_mutex(&state_mutex, astroturkv2_state);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);


    return 0;
}
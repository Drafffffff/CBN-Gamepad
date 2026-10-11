/* Runs only in an isolated process/profile. Synthesizes SDL3 controller input and
 * captures the actual native renderer; never preload into a user's live game. */
#define _GNU_SOURCE
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SDL_JoystickID fixture;
static Uint64 begin;
static unsigned stage, saved;
static Sint16 axes[SDL_GAMEPAD_AXIS_COUNT];
bool SDL_InitSubSystem(SDL_InitFlags flags) {
    bool (*real_init)(SDL_InitFlags) = dlsym(RTLD_NEXT,"SDL_InitSubSystem");
    bool ok=real_init(flags);
    if(ok && (flags & SDL_INIT_GAMEPAD) && !fixture) {
        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;
        desc.name="CBN isolated controller fixture";
        fixture=SDL_AttachVirtualJoystick(&desc);
        fprintf(stderr,"Fixture id=%u gamepad=%d\n",fixture,SDL_IsGamepad(fixture));
    }
    return ok;
}
SDL_JoystickID *SDL_GetGamepads(int *count) {
    SDL_JoystickID *(*real_get)(int *)=dlsym(RTLD_NEXT,"SDL_GetGamepads");
    if(!fixture)return real_get(count);
    SDL_JoystickID *ids=SDL_malloc(2*sizeof(*ids));
    ids[0]=fixture;ids[1]=0;if(count)*count=1;return ids;
}
Sint16 SDL_GetGamepadAxis(SDL_Gamepad *pad,SDL_GamepadAxis axis) {
    Sint16 (*real_axis)(SDL_Gamepad *,SDL_GamepadAxis)=dlsym(RTLD_NEXT,"SDL_GetGamepadAxis");
    return SDL_GetGamepadID(pad)==fixture ? axes[axis] : real_axis(pad,axis);
}
struct step { Uint64 at; int kind; int key; int value; };
static const struct step steps[]={
    {2000,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,24000},
    {2050,1,SDL_GAMEPAD_BUTTON_START,1},{2100,1,SDL_GAMEPAD_BUTTON_START,0},
    {2200,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0},
    {4500,1,SDL_GAMEPAD_BUTTON_EAST,1},{4550,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {6000,3,'n',0},
    {6300,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,1},{6350,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,0},
    {6600,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,1},{6650,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,0},
    {6900,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,1},{6950,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,0},
    {7300,1,SDL_GAMEPAD_BUTTON_SOUTH,1},{7350,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {9000,1,SDL_GAMEPAD_BUTTON_SOUTH,1},{9050,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {60000,4,SDL_GAMEPAD_BUTTON_BACK,1},{60050,4,SDL_GAMEPAD_BUTTON_BACK,0},
    {65000,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,24000},{65100,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {68000,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,1},{68050,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,0},
    {68100,0,SDL_GAMEPAD_AXIS_LEFTY,0},{68200,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {70500,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,1},{70550,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,0},
    {70600,0,SDL_GAMEPAD_AXIS_LEFTY,0},{70700,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {73000,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,1},{73050,1,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,0},
    {73100,0,SDL_GAMEPAD_AXIS_LEFTY,0},{73200,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {75000,2,SDLK_RIGHT,0},
    {77000,1,SDL_GAMEPAD_BUTTON_EAST,1},{77050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {77100,0,SDL_GAMEPAD_AXIS_LEFTY,0},{77200,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0},
    {80000,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,1},{80050,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,0},
    {82000,1,SDL_GAMEPAD_BUTTON_EAST,1},{82050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {85000,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,24000},
    {85100,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,1},{85150,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,0},
    {85200,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0},
    {88000,1,SDL_GAMEPAD_BUTTON_SOUTH,1},{88050,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {91000,1,SDL_GAMEPAD_BUTTON_EAST,1},{91050,1,SDL_GAMEPAD_BUTTON_EAST,0}
};
bool SDL_PollEvent(SDL_Event *event) {
    bool (*real_poll)(SDL_Event *)=dlsym(RTLD_NEXT,"SDL_PollEvent");
    if(!begin && fixture)begin=SDL_GetTicks();
    while(real_poll(event)) {
        if(event && ((event->type>=SDL_EVENT_KEY_DOWN && event->type<=SDL_EVENT_KEYMAP_CHANGED) ||
          (event->type>=SDL_EVENT_MOUSE_MOTION && event->type<=SDL_EVENT_MOUSE_WHEEL) ||
          event->type==SDL_EVENT_GAMEPAD_BUTTON_DOWN || event->type==SDL_EVENT_GAMEPAD_BUTTON_UP ||
          event->type==SDL_EVENT_GAMEPAD_AXIS_MOTION))continue;
        return true;
    }
    if(!begin || !event)return false;
    Uint64 elapsed=SDL_GetTicks()-begin;
    if(getenv("CBN_SMOKE_MENU_ONLY") && elapsed>5000)_Exit((saved&3)==3 ? 0 : 3);
    if(elapsed>95000){fprintf(stderr,"Fixture screenshots=%u expected=8191\n",saved);_Exit(saved==8191 ? 0 : 3);}
    if(stage<sizeof(steps)/sizeof(*steps) && elapsed>steps[stage].at) {
        struct step step=steps[stage++];
        if(step.kind==4){if(!getenv("CBN_SMOKE_MAP"))return false;step.kind=1;}
        if(getenv("CBN_SMOKE_MAP") && (step.at==80000 || step.at==80050))step.key=SDL_GAMEPAD_BUTTON_BACK;
        fprintf(stderr,"step=%u elapsed=%llu kind=%d key=%d\n",stage,(unsigned long long)elapsed,step.kind,step.key);
        memset(event,0,sizeof(*event));
        if(step.kind==0){axes[step.key]=step.value;event->type=SDL_EVENT_GAMEPAD_AXIS_MOTION;event->gaxis.which=fixture;event->gaxis.axis=step.key;event->gaxis.value=step.value;}
        else if(step.kind==1){event->type=step.value?SDL_EVENT_GAMEPAD_BUTTON_DOWN:SDL_EVENT_GAMEPAD_BUTTON_UP;event->gbutton.which=fixture;event->gbutton.button=step.key;}
        else if(step.kind==3){static char text[2];text[0]=step.key;text[1]=0;event->type=SDL_EVENT_TEXT_INPUT;event->text.text=text;}
        else {event->type=SDL_EVENT_KEY_DOWN;event->key.key=step.key;event->key.down=true;}
        return true;
    }
    static Uint64 exposed;
    if(elapsed-exposed>250){exposed=elapsed;memset(event,0,sizeof(*event));event->type=SDL_EVENT_WINDOW_EXPOSED;return true;}
    return false;
}
bool SDL_RenderPresent(SDL_Renderer *renderer) {
    bool (*real_present)(SDL_Renderer *)=dlsym(RTLD_NEXT,"SDL_RenderPresent");
    static const Uint64 times[]={1000,3000,62000,66500,69000,71500,74000,77500,80500,83500,87000,90000,92500};
    static const char *names[]={"main-menu","controller-help","world","explore","items","survival","information","cancelled","look","look-returned","nearby-items","advanced-inventory","inventory-returned"};
    Uint64 elapsed=begin ? SDL_GetTicks()-begin : 0;
    for(unsigned i=0;i<sizeof(times)/sizeof(*times);i++) {
        if(elapsed>times[i] && elapsed<times[i]+2000 && !(saved&(1u<<i))) {
            SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);
            if(surface){char path[1024];snprintf(path,sizeof(path),"%s/%s.png",getenv("CBN_SMOKE_OUTPUT"),names[i]);if(IMG_SavePNG(surface,path)){saved|=1u<<i;fprintf(stderr,"capture=%s elapsed=%llu\n",names[i],(unsigned long long)elapsed);}SDL_DestroySurface(surface);}
            break;
        }
    }
    return real_present(renderer);
}

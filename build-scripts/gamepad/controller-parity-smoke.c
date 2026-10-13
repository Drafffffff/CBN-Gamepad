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
    {7300,1,SDL_GAMEPAD_BUTTON_SOUTH,1},{7350,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {9000,1,SDL_GAMEPAD_BUTTON_SOUTH,1},{9050,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {61000,0,SDL_GAMEPAD_AXIS_LEFTX,0},
    {61010,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {62000,0,SDL_GAMEPAD_AXIS_LEFTX,26000},
    {62010,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {63000,0,SDL_GAMEPAD_AXIS_LEFTX,26000},
    {63010,0,SDL_GAMEPAD_AXIS_LEFTY,0},
    {64000,0,SDL_GAMEPAD_AXIS_LEFTX,26000},
    {64010,0,SDL_GAMEPAD_AXIS_LEFTY,26000},
    {65000,0,SDL_GAMEPAD_AXIS_LEFTX,0},
    {65010,0,SDL_GAMEPAD_AXIS_LEFTY,26000},
    {66000,0,SDL_GAMEPAD_AXIS_LEFTX,-26000},
    {66010,0,SDL_GAMEPAD_AXIS_LEFTY,26000},
    {67000,0,SDL_GAMEPAD_AXIS_LEFTX,-26000},
    {67010,0,SDL_GAMEPAD_AXIS_LEFTY,0},
    {68000,0,SDL_GAMEPAD_AXIS_LEFTX,-26000},
    {68010,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {69000,0,SDL_GAMEPAD_AXIS_LEFTX,0},
    {69010,0,SDL_GAMEPAD_AXIS_LEFTY,0},
    {70000,0,SDL_GAMEPAD_AXIS_LEFTX,26000},
    {70400,0,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,24000},
    {70460,0,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,0},
    {71000,0,SDL_GAMEPAD_AXIS_LEFTX,0},
    {73000,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,1},
    {73050,1,SDL_GAMEPAD_BUTTON_RIGHT_STICK,0},
    {74000,0,SDL_GAMEPAD_AXIS_RIGHTX,26000},
    {74010,0,SDL_GAMEPAD_AXIS_RIGHTY,-26000},
    {74100,0,SDL_GAMEPAD_AXIS_RIGHTX,0},
    {74110,0,SDL_GAMEPAD_AXIS_RIGHTY,0},
    {75000,1,SDL_GAMEPAD_BUTTON_EAST,1},
    {75050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {77000,1,SDL_GAMEPAD_BUTTON_EAST,1},
    {77050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {78000,1,SDL_GAMEPAD_BUTTON_EAST,1},
    {78050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {80000,1,SDL_GAMEPAD_BUTTON_WEST,1},
    {80050,1,SDL_GAMEPAD_BUTTON_WEST,0},
    {81000,1,SDL_GAMEPAD_BUTTON_EAST,1},
    {81050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {83000,0,SDL_GAMEPAD_AXIS_LEFTX,26000},
    {83500,1,SDL_GAMEPAD_BUTTON_SOUTH,1},
    {83550,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {84000,0,SDL_GAMEPAD_AXIS_LEFTX,0},
    {87000,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,24000},
    {87060,1,SDL_GAMEPAD_BUTTON_START,1},
    {87110,1,SDL_GAMEPAD_BUTTON_START,0},
    {87200,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0},
    {88700,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,1},
    {88750,1,SDL_GAMEPAD_BUTTON_DPAD_DOWN,0},
    {89000,1,SDL_GAMEPAD_BUTTON_SOUTH,1},
    {89050,1,SDL_GAMEPAD_BUTTON_SOUTH,0},
    {91000,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,24000},
    {91100,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {93000,1,SDL_GAMEPAD_BUTTON_EAST,1},
    {93050,1,SDL_GAMEPAD_BUTTON_EAST,0},
    {93100,0,SDL_GAMEPAD_AXIS_LEFTY,0},
    {93200,0,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0},
};
bool SDL_PollEvent(SDL_Event *event) {
    bool (*real_poll)(SDL_Event *)=dlsym(RTLD_NEXT,"SDL_PollEvent");
    static bool noise_pending;
    if(event && noise_pending) {
        noise_pending=false;
        memset(event,0,sizeof(*event));event->type=SDL_EVENT_MOUSE_MOTION;
        event->motion.x=200;event->motion.y=120;
        return true;
    }
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
    if(elapsed>97000){fprintf(stderr,"Fixture screenshots=%u expected=16777215\n",saved);_Exit(saved==16777215 ? 0 : 3);}
    if(stage<sizeof(steps)/sizeof(*steps) && elapsed>steps[stage].at) {
        struct step step=steps[stage++];
        if(getenv("CBN_SMOKE_MENU_NAV")) {
            if(step.at==6000)return false;
            if(step.at==6300 || step.at==6350 || step.at==6600 || step.at==6650) {
                step.kind=0;step.key=SDL_GAMEPAD_AXIS_LEFTY;
                step.value=(step.at==6300 || step.at==6600) ? 26000 : 0;
            }
        }
        if(step.kind==4){if(!getenv("CBN_SMOKE_MAP"))return false;step.kind=1;}
        if(getenv("CBN_SMOKE_MAP") && (step.at==80000 || step.at==80050))step.key=SDL_GAMEPAD_BUTTON_BACK;
        fprintf(stderr,"step=%u elapsed=%llu kind=%d key=%d\n",stage,(unsigned long long)elapsed,step.kind,step.key);
        memset(event,0,sizeof(*event));
        if(step.kind==0){axes[step.key]=step.value;event->type=SDL_EVENT_GAMEPAD_AXIS_MOTION;event->gaxis.which=fixture;event->gaxis.axis=step.key;event->gaxis.value=step.value;}
        else if(step.kind==1){event->type=step.value?SDL_EVENT_GAMEPAD_BUTTON_DOWN:SDL_EVENT_GAMEPAD_BUTTON_UP;event->gbutton.which=fixture;event->gbutton.button=step.key;}
        else if(step.kind==3){static char text[2];text[0]=step.key;text[1]=0;event->type=SDL_EVENT_TEXT_INPUT;event->text.text=text;}
        else {event->type=SDL_EVENT_KEY_DOWN;event->key.key=step.key;event->key.down=true;}
        if(getenv("CBN_SMOKE_MIXED_INPUT") && (step.kind==0 || step.kind==1))noise_pending=true;
        return true;
    }
    static Uint64 exposed;
    if(elapsed-exposed>250){exposed=elapsed;memset(event,0,sizeof(*event));event->type=SDL_EVENT_WINDOW_EXPOSED;return true;}
    return false;
}
bool SDL_RenderPresent(SDL_Renderer *renderer) {
    bool (*real_present)(SDL_Renderer *)=dlsym(RTLD_NEXT,"SDL_RenderPresent");
    static const Uint64 times[]={60000,61300,62300,63300,64300,65300,66300,67300,68300,69300,71300,73300,74300,75300,77300,78300,80300,81300,83700,85000,88000,89500,92000,94000};
    static const char *names[]={"baseline","preview-n","preview-ne","preview-e","preview-se","preview-s","preview-sw","preview-w","preview-nw","centered","move-confirmed","look-open","look-diagonal","look-returned","grab-direction","grab-returned","smash-direction","smash-returned","empty-tile-interact","interact-returned","action-palette","palette-executed","wheel-open","wheel-returned"};
    Uint64 elapsed=begin ? SDL_GetTicks()-begin : 0;
    for(unsigned i=0;i<sizeof(times)/sizeof(*times);i++) {
        if(elapsed>times[i] && elapsed<times[i]+700 && !(saved&(1u<<i))) {
            SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);
            if(surface){char path[1024];snprintf(path,sizeof(path),"%s/%s.png",getenv("CBN_SMOKE_OUTPUT"),names[i]);if(IMG_SavePNG(surface,path)){saved|=1u<<i;fprintf(stderr,"capture=%s elapsed=%llu\n",names[i],(unsigned long long)elapsed);}SDL_DestroySurface(surface);}
            break;
        }
    }
    return real_present(renderer);
}

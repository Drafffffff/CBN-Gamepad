/* Runs only in an isolated process/profile. Synthesizes SDL3 controller input and
 * captures the actual native renderer; never preload into a user's live game. */
#define _GNU_SOURCE
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
static SDL_JoystickID fixture;
static Uint64 begin;
static unsigned stage, saved;
static Sint16 axes[SDL_GAMEPAD_AXIS_COUNT];
bool SDL_InitSubSystem(SDL_InitFlags flags) {
    bool (*real_init)(SDL_InitFlags) = dlsym(RTLD_NEXT,"SDL_InitSubSystem");
    bool ok=real_init(flags);
    if(ok && (flags & SDL_INIT_GAMEPAD) && !fixture && !getenv("CBN_SMOKE_HOTPLUG")) {
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
    if(!fixture){if(getenv("CBN_SMOKE_HOTPLUG")){if(count)*count=0;return NULL;}return real_get(count);}
    SDL_JoystickID *ids=SDL_malloc(2*sizeof(*ids));
    ids[0]=fixture;ids[1]=0;if(count)*count=1;return ids;
}
Sint16 SDL_GetGamepadAxis(SDL_Gamepad *pad,SDL_GamepadAxis axis) {
    Sint16 (*real_axis)(SDL_Gamepad *,SDL_GamepadAxis)=dlsym(RTLD_NEXT,"SDL_GetGamepadAxis");
    return SDL_GetGamepadID(pad)==fixture ? axes[axis] : real_axis(pad,axis);
}
struct step { Uint64 at; int kind; int key; int value; };
static const struct step steps[]={
    {1800,5,0,0},
    {6000,3,'L',0},
    {7300,2,SDLK_RETURN,0},
    {9000,2,SDLK_RETURN,0},
    {22000,5,0,0},
    {22500,0,SDL_GAMEPAD_AXIS_RIGHTX,26000},
    {22600,0,SDL_GAMEPAD_AXIS_RIGHTX,0},
    {24000,0,SDL_GAMEPAD_AXIS_LEFTY,-26000},
    {28000,0,SDL_GAMEPAD_AXIS_LEFTY,0}

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
    if(!begin)begin=SDL_GetTicks();
    Uint64 hotplug_elapsed=SDL_GetTicks()-begin;
    static unsigned connection_stage;
    if(getenv("CBN_SMOKE_HOTPLUG") && ((connection_stage==0 && hotplug_elapsed>1500) || (connection_stage==2 && hotplug_elapsed>36000))) {
        SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
        desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="CBN hotplug fixture";
        fixture=SDL_AttachVirtualJoystick(&desc);connection_stage++;
        fprintf(stderr,"hotplug attached=%u elapsed=%llu\n",fixture,(unsigned long long)hotplug_elapsed);
    }
    if(getenv("CBN_SMOKE_HOTPLUG") && connection_stage==1 && hotplug_elapsed>32000) {
        SDL_DetachVirtualJoystick(fixture);fixture=0;connection_stage++;
        fprintf(stderr,"hotplug detached elapsed=%llu\n",(unsigned long long)hotplug_elapsed);
    }
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
    if(elapsed>40000){fprintf(stderr,"Fixture screenshots=%u expected=255\n",saved);_Exit(saved==255 ? 0 : 3);}
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
        if(step.kind==5){event->type=SDL_EVENT_WINDOW_FOCUS_GAINED;}
        else if(step.kind==0){axes[step.key]=step.value;event->type=SDL_EVENT_GAMEPAD_AXIS_MOTION;event->gaxis.which=fixture;event->gaxis.axis=step.key;event->gaxis.value=step.value;}
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
    const bool presented=real_present(renderer);
    static const Uint64 times[]={1000,3000,22000,23500,26000,30000,34000,38000};
    static const char *names[]={"before-connect","connected-footer","world-log-footer","right-camera","left-target-priority","left-centered-camera","disconnected-log","reconnected-log"};
    Uint64 elapsed=begin ? SDL_GetTicks()-begin : 0;
    for(unsigned i=0;i<sizeof(times)/sizeof(*times);i++) {
        if(elapsed>times[i] && elapsed<times[i]+1500 && !(saved&(1u<<i))) {
            char path[1024];
            snprintf(path,sizeof(path),"%s/%s.png",getenv("CBN_SMOKE_OUTPUT"),names[i]);
            pid_t child=fork();
            if(child==0){unsetenv("LD_PRELOAD");unsetenv("LD_LIBRARY_PATH");execlp("import","import","-window","root",path,(char *)NULL);_Exit(127);}
            int status;
            if(child>0 && waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0){saved|=1u<<i;fprintf(stderr,"capture=%s elapsed=%llu\n",names[i],(unsigned long long)elapsed);}

            break;
        }
    }
    return presented;
}

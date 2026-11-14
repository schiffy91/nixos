local source = assert(arg[1])
local focus, observer
local keyboard = {shown=false}
ConsolePortKeyboard = keyboard
function keyboard:OnFocusChanged(frame) self.focus=frame;self.shown=not not frame end
function keyboard:IsShown() return self.shown end
function keyboard:SetState(state) assert(state==1) end
function keyboard:OnTextChanged() end
function GetCurrentKeyBoardFocus() return focus end
function IsShiftKeyDown() return false end
IsControlKeyDown, IsAltKeyDown = IsShiftKeyDown, IsShiftKeyDown
bit = {bor=function(...) return 0 end}
function CreateFrame()
    observer={scripts={}}
    function observer:EnableKeyboard(value) self.keyboard=value end
    function observer:EnableGamePadButton(value) self.gamepad=value end
    function observer:SetPropagateKeyboardInput(value)
        assert(value, 'the observer must never consume keyboard or gamepad input')
        self.propagates=value
    end
    function observer:SetScript(event,fn) self.scripts[event]=fn end
    return observer
end
local function editbox()
    local box={hooks={}}
    function box:IsForbidden() return false end
    function box:IsAnchoringRestricted() return false end
    function box:GetAttribute() return self.hidden end
    function box:GetText() return 'example' end
    function box:GetUTF8CursorPosition() return 7 end
    function box:HookScript(event,fn)
        assert(not self.hooks[event], 'do not accumulate hooks as focus changes')
        self.hooks[event]=fn
    end
    return box
end
local env={Attributes={HideKeyboard='hidekeyboard'}}
assert(loadfile(source))('ConsolePort_Keyboard',env)
env:ToggleObserver(true)
local function update() observer.scripts.OnUpdate(observer,0.01) end
local function key(key) observer.scripts.OnKeyDown(observer,key) end
local function pad() observer.scripts.OnGamePadButtonDown(observer,'PAD1');update() end
assert(observer.keyboard and observer.gamepad and observer.propagates)

key('ENTER');focus=editbox();update()
assert(not keyboard.shown, 'opening chat with a physical keyboard must not show the virtual keyboard')
pad()
assert(keyboard.shown and keyboard.focus==focus, 'controller input can resume typing in the same edit box')
key('LSHIFT');key('RCTRL');update()
assert(keyboard.shown, 'trigger-emulated modifiers must not be mistaken for physical typing')
key('A');update()
assert(not keyboard.shown, 'physical typing dismisses the virtual keyboard')
focus=nil;update();focus=editbox();update()
assert(not keyboard.shown, 'physical input mode persists across edit boxes')
pad();assert(keyboard.shown)
focus.hooks.OnChar(focus,'a');update()
assert(not keyboard.shown, 'edit-box character input works even if another frame consumed OnKeyDown')
local previous=focus
focus=nil;update();focus=previous;update();pad()
assert(keyboard.shown, 'returning to a previously observed box keeps controller typing available')
focus.hidden=true;update();key('ENTER');pad()
assert(not keyboard.shown, 'respect edit boxes that explicitly hide the keyboard')
print('PASS: physical typing, controller resume, modifier keys, edit-box hooks and input propagation')

local source=assert(arg[1])
local width,height,combat,editing=6016,3384,false,false
local callbacks,frames,timers
local current='Desk / Controller'
local enabled,loaded,dialogs=true,true,0
local profiles={}
for display,scale in pairs({Desk=.64,TV=768/900,Handheld=.96}) do
    for _,input in ipairs({'Controller','Keyboard'}) do
        profiles[display..' / '..input]={displayLayout={enabled=true,scale=scale,input=input:lower()}}
    end
end
profiles.Default=profiles['Handheld / Controller']
local db={sv={profiles=profiles},global={uiScale='custom',uiScaleCustom=.64},profile=profiles[current]}
local ns={db=db,ShowDialog=function() dialogs=dialogs+1 end}
function db:GetCurrentProfile() return current end
function db:SetProfile(name)
    current=name;self.profile=profiles[name]
    for _,fn in ipairs(callbacks.OnProfileChanged or {}) do fn() end
end
function db.RegisterCallback(_,event,fn)
    callbacks[event]=callbacks[event] or {};table.insert(callbacks[event],fn)
end
local editMode={RegisterCallback=db.RegisterCallback,IsInEditMode=function() return editing end}
function LibStub() return editMode end
function InCombatLockdown() return combat end
function GetPhysicalScreenSize() return width,height end
function UnitGUID() return 'Player-91-00001234' end
function hooksecurefunc() end
C_AddOns={EnableAddOn=function() enabled=true end,DisableAddOn=function() enabled=false end,
    IsAddOnLoaded=function() return loaded end}
C_Timer={After=function(_,fn) table.insert(timers,fn) end}
function CreateFrame()
    local f={RegisterEvent=function() end,SetScript=function(self,_,fn) self.event=fn end}
    table.insert(frames,f);return f
end
local function flush()
    while #timers>0 do local pending=timers;timers={};for _,fn in ipairs(pending) do fn() end end
end
local function event(name) for _,f in ipairs(frames) do f:event(name) end;flush() end
local function reload()
    -- A real reload recreates addon locals and callbacks, preserves SavedVariables,
    -- and loads the addons enabled before the reload.
    callbacks,frames,timers={},{},{}
    loaded,dialogs=enabled,0
    assert(loadfile(source))('FlareUI',ns)
    event('DISPLAY_SIZE_CHANGED')
    event('PLAYER_LOGIN')
    event('DISPLAY_SIZE_CHANGED')
end
reload();assert(current=='Desk / Controller' and dialogs==0)
db:SetProfile('Handheld / Controller');assert(dialogs==1 and db.global.uiScaleCustom==.96)
for i=1,2 do reload();assert(current=='Handheld / Controller' and dialogs==0,'manual Handheld selection must survive repeated reloads on a desk display') end
width,height=3840,2160
reload();assert(current=='TV / Controller' and dialogs==1,'detect a real display change while the game was closed')
reload();assert(current=='TV / Controller' and dialogs==0,'automatic display choice must settle after one reload')
db:SetProfile('Handheld / Keyboard');assert(not enabled and dialogs==1)
for i=1,2 do reload();assert(current=='Handheld / Keyboard' and not loaded and dialogs==0,'manual display and input choice must both survive reload') end
width,height=6016,3384
event('DISPLAY_SIZE_CHANGED');assert(current=='Desk / Keyboard' and dialogs==1)
reload();assert(current=='Desk / Keyboard' and dialogs==0)
combat=true;db:SetProfile('Handheld / Keyboard');assert(dialogs==0)
combat=false;event('PLAYER_REGEN_ENABLED');assert(current=='Handheld / Keyboard' and dialogs==1,'combat deferral must not replace a manual selection')
reload();assert(current=='Handheld / Keyboard' and dialogs==0)
editing=true;db:SetProfile('TV / Keyboard');assert(dialogs==0)
editing=false;for _,fn in ipairs(callbacks.exit) do fn() end;flush()
assert(current=='TV / Keyboard' and dialogs==1,'Edit Mode deferral must preserve manual intent')
reload();assert(current=='TV / Keyboard' and dialogs==0)
combat=true;width,height=1280,800;event('DISPLAY_SIZE_CHANGED')
assert(current=='TV / Keyboard')
combat=false;event('PLAYER_REGEN_ENABLED');assert(current=='Handheld / Keyboard' and dialogs==1)
reload();assert(current=='Handheld / Keyboard' and dialogs==0)
current='Default';db.profile=profiles.Default;db.global.displayLayoutScreen=nil
db.global.uiScaleCustom=.96;enabled=true;width,height=6016,3384
reload();assert(current=='Desk / Controller' and dialogs==1,'new Default profiles should select the initial physical display')
reload();assert(dialogs==0)
print('PASS: manual profiles survive reloads, real display changes preserve input mode, and deferred changes settle without reload loops')

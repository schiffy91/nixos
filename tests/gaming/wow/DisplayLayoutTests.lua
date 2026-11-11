local source = assert(arg[1])
local callbacks, frames, timers = {}, {}, {}
local width, height, combat, editing = 1280, 800, false, false
local loaded, enabled, dialogs, chatWrites = true, true, 0, 0
local cvars = { GamePadEmulateShift = 'none' }
function GetCVar(name) return cvars[name] end
function SetCVar(name, value) assert(cvars[name] ~= nil); cvars[name] = value end
local current = 'Handheld / Controller'
local profiles = {}
for name, scale in pairs({ Handheld = .96, TV = 768/900, Desk = .64 }) do
    for _, input in ipairs({'Keyboard', 'Controller'}) do
        profiles[name .. ' / ' .. input] = { displayLayout = {
            enabled = true, scale = scale, input = input:lower(),
            chat = {x = 24, y = 24, width = 320, height = 150},
            cvars = {GamePadEmulateShift='PADLTRIGGER', unavailable='1'},
            tooltip = {gap=12,y=0},
            toolbar = input=='Controller' and {height=44,xpHeight=8,bottom=0,padding=2,minGap=4,iconWidth=28} or nil,
        } }
    end
end
local db = { sv = { profiles = profiles }, global = {uiScale='custom', uiScaleCustom=.96}, profile=profiles[current] }
local ns = { db = db, ShowDialog = function() dialogs = dialogs + 1 end }
function db:GetCurrentProfile() return current end
function db:SetProfile(name)
    current = name; self.profile = self.sv.profiles[name]
    for _, fn in ipairs(callbacks.OnProfileChanged or {}) do fn() end
end
function db.RegisterCallback(_, event, fn)
    callbacks[event] = callbacks[event] or {}; table.insert(callbacks[event], fn)
end
local editMode = { RegisterCallback = db.RegisterCallback }
function editMode:IsInEditMode() return editing end
function LibStub(name) assert(name == 'FlareEditMode'); return editMode end
function InCombatLockdown() return combat end
function GetPhysicalScreenSize() return width, height end
function UnitName() error('Forever addon state requires a GUID, not a name') end
function UnitGUID() return 'Player-91-00001234' end
C_AddOns = {
    EnableAddOn = function(name, character) assert(name=='ConsolePort_Bar' and character=='Player-91-00001234'); enabled=true end,
    DisableAddOn = function(name, character) assert(name=='ConsolePort_Bar' and character=='Player-91-00001234'); enabled=false end,
    IsAddOnLoaded = function() return loaded end,
}
local children={}
for i=1,12 do children[i]={GetWidth=function() return 32 end,GetHeight=function() return 36 end} end
local grid={GetLayoutChildren=function() return children end,SetScale=function(self,s) self.scale=s end,
    Layout=function(self) self.width=#children*32+(#children-1)*self.childXPadding; ConsolePortToolbar:OnGridSizeChanged() end}
local border={SetAlpha=function(self,a) self.alpha=a end,ClearAllPoints=function() end,SetPoint=function(self,point,relative) self[point]=relative end}
local menu={Grid=grid,Border=border,SetScale=function() end,SetSize=function(self,w,h) self.width,self.height=w,h end,
    ClearAllPoints=function() end,SetPoint=function(self,point,relative,relativePoint,x,y) self.anchor={point,relative,relativePoint,x,y} end,
    HookScript=function(self,_,fn) self.onShow=fn end}
local displayScale=1
ConsolePortToolbar={Menu=menu,GetEffectiveScale=function() return displayScale end,
    OnGridSizeChanged=function() end,UpdateDirection=function() end,
    Notch={SetHoverZone=function(self,w,h) self.width,self.height=w,h end,ClearAllPoints=function() end,SetPoint=function() end}}
local watchY, stripLow, stripHeight, triggerY = 0, -3, 14, 100
local function background() return {Hide=function(self) self.hidden=true end,Show=function(self) self.hidden=false end,
    SetAllPoints=function(self,relative) self.relative=relative end,SetColorTexture=function(self,...) self.color={...} end} end
local rested={SetHeight=function(self,h) self.height=h end}
local status={Background=background(),GetHeight=function() return ConsolePortWatchbar.XPBar.barHeight end,
    GetWidth=function() return ConsolePortWatchbar.XPBar.width-8 end}
local strip={IsShown=function() return true end,GetBottom=function() return watchY+stripLow end,
    GetTop=function() return watchY+stripLow+stripHeight end,GetEffectiveScale=function() return .8*displayScale end}
ConsolePortWatchbar={GetWidth=function() return 700 end,
    XPBar={width=560,GetWidth=function(self) return self.width end,SetWidth=function(self,w) self.width=w end,
    GetEndCapWidth=function() return 4 end,bars={{StatusBar=status,ExhaustionLevelFillBar=rested,
    CreateTexture=function(_,name,layer,template,sublevel) assert(layer=='BACKGROUND' and sublevel==-2);return background() end}},GetHeight=function() return 16 end,
    GetEffectiveScale=function() return .8*displayScale end,SingleBarLarge=strip,LayoutBars=function() end,
    BGLeft=background(),BGRight=background(),barHeight=14,
    UpdateBarsShown=function(self) stripHeight=self.barHeight;self:LayoutBars() end},
    GetBottom=function() return watchY end,GetEffectiveScale=function() return .8*displayScale end,
    ClearAllPoints=function() end,SetPoint=function(_,_,_,_,_,y) watchY=y end,SetProps=function() end}
CPB_PADLTRIGGER={GetBottom=function() return triggerY end,GetEffectiveScale=function() return .8*displayScale end}
CPB_PADRTRIGGER={GetBottom=function() return triggerY+10 end,GetEffectiveScale=function() return .8*displayScale end}
ConsolePortBarCluster={SetProps=function() end,GetEffectiveScale=function() return .8*displayScale end,
    GetPoint=function() return 'BOTTOM',UIParent,'BOTTOM',0,triggerY end,
    SetPoint=function(_,_,_,_,_,y) triggerY=y end}
function hooksecurefunc(object,key,callback)
    if type(object)=="string" then object,key,callback=_G,object,key end
    local original=object[key]; object[key]=function(...) original(...); callback(...) end
end
GameTooltip_SetDefaultAnchor=function() end
MinimapCluster={}
GameTooltip={IsForbidden=function() return false end,SetOwner=function(self,_,mode) self.mode=mode end,
    ClearAllPoints=function() end,SetPoint=function(self,...) self.anchor={...} end}
C_Timer = {After=function(_, fn) table.insert(timers, fn) end}
UIParent = {GetEffectiveScale=function() return displayScale end}
ChatFrame1 = {ClearAllPoints=function() end, SetPoint=function() end,
    SetSize=function() chatWrites=chatWrites+1 end, GetEffectiveScale=function() return 1 end,
    GetRect=function() return 30,40,310,140 end}
function FCF_SavePositionAndDimensions() end
function CreateFrame()
    local frame={RegisterEvent=function() end, SetScript=function(self, _, fn) self.event=fn end}
    table.insert(frames,frame); return frame
end
local function flush()
    while #timers > 0 do local pending=timers; timers={}; for _, fn in ipairs(pending) do fn() end end
end
local function event(name) for _, f in ipairs(frames) do f:event(name) end; flush() end
assert(loadfile(source))('FlareUI',ns)
event('DISPLAY_SIZE_CHANGED'); assert(dialogs==0 and chatWrites==0)
event('PLAYER_LOGIN'); assert(dialogs==0 and chatWrites==1)
assert(menu.width==355.25 and menu.height==33.5 and menu.anchor[2]==UIParent and menu.anchor[5]==0)
local function assertMatchingBars()
    assert(math.abs(grid.width*grid.scale-menu.width)<.001 and grid.childXPadding==2, 'menu must fit its actual icon row')
    assert(math.abs(status:GetWidth()*.8-menu.width)<.001, 'XP progress track must match the visible menu row')
    assert(rested.height==status:GetHeight(), 'rested and main XP fills must have the same height')
    local track=ConsolePortWatchbar.XPBar.bars[1].displayLayoutTrack
    assert(track.relative==status and not track.hidden and status.Background.hidden, 'show the full-width empty track below rested XP')
end
assertMatchingBars()
for i=1,10 do menu.onShow();assertMatchingBars() end
children[13]=children[1];menu.onShow();assertMatchingBars()
children[13]=nil;menu.onShow();assertMatchingBars()
assert(ConsolePortWatchbar.XPBar.barHeight*.8==8 and stripHeight*.8==8, 'native XP strip height must be reduced')
assert(border.alpha==0 and ConsolePortWatchbar.XPBar.BGLeft.hidden and ConsolePortWatchbar.XPBar.BGRight.hidden, 'remove both sources of the black background')
assert(border.TOPLEFT==menu and border.BOTTOMRIGHT==menu, 'border must not hang over the XP bar')
assert(math.abs(36*grid.scale+2-menu.height)<.001, 'menu height must fit below the trigger buttons')
local function assertEqualGaps()
    local xpBottom=(watchY+stripLow)*.8
    local xpTop=xpBottom+stripHeight*.8
    local lowerGap=xpBottom-menu.height
    local upperGap=triggerY*.8-xpTop
    assert(lowerGap>=4-.001 and math.abs(lowerGap-upperGap)<.001, 'XP strip must split the free space evenly below LT/RT')
end
assertEqualGaps()
for _,scale in ipairs({.64,768/900,.96}) do
    displayScale=scale
    ConsolePortWatchbar:SetProps()
    assertEqualGaps();assertMatchingBars()
end
stripLow,stripHeight=-9,26
ConsolePortWatchbar.XPBar:LayoutBars()
assertEqualGaps()
triggerY=70
ConsolePortBarCluster:SetProps()
assertEqualGaps()
assert(menu.height==33.5 and triggerY>70, 'raise the cluster rather than shrink the menu when a second watch bar needs space')
local previousY=watchY
combat=true;triggerY=110;ConsolePortBarCluster:SetProps()
assert(watchY==previousY, 'defer layout during combat')
combat=false;event('PLAYER_REGEN_ENABLED');assertEqualGaps()
GameTooltip_SetDefaultAnchor(GameTooltip, UIParent)
assert(GameTooltip.mode=='ANCHOR_NONE' and GameTooltip.anchor[1]=='BOTTOMRIGHT' and GameTooltip.anchor[2]==MinimapCluster and GameTooltip.anchor[3]=='BOTTOMLEFT' and GameTooltip.anchor[4]==-12)
assert(cvars.GamePadEmulateShift=='PADLTRIGGER', 'restore shared gameplay CVars without touching unavailable ones')
event('PLAYER_REGEN_ENABLED'); assert(chatWrites==1, 'combat exit must not reset user chat edits')
width,height=3840,2160
event('DISPLAY_SIZE_CHANGED'); assert(current=='TV / Controller' and db.global.uiScaleCustom==768/900 and enabled)
local shown=dialogs
event('DISPLAY_SIZE_CHANGED'); assert(dialogs==shown, 'repeated display events must not repeat the prompt')
db:SetProfile('TV / Keyboard'); assert(not enabled, 'keyboard profile must disable only ConsolePort_Bar')
combat=true; width,height=6016,3384
event('DISPLAY_SIZE_CHANGED'); assert(current=='TV / Keyboard', 'do not switch during combat')
combat=false; event('PLAYER_REGEN_ENABLED'); assert(current=='Desk / Keyboard' and db.global.uiScaleCustom==.64 and not enabled)
editing=true; width,height=1280,800
event('DISPLAY_SIZE_CHANGED'); assert(current=='Desk / Keyboard', 'defer switching until Edit Mode closes')
editing=false; for _, fn in ipairs(callbacks.exit) do fn() end; flush()
assert(current=='Handheld / Keyboard')
profiles.Unmanaged={};db:SetProfile('Unmanaged');shown=dialogs;width,height=3840,2160
event('DISPLAY_SIZE_CHANGED');assert(current=='Unmanaged' and dialogs==shown,'unmanaged profiles opt out')
print('PASS: display selection, input preservation, reload requests, combat/Edit Mode deferral and opt-out')

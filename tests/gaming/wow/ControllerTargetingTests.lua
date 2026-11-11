local source = assert(arg[1])
local combat, secure, identityRestricted = false, false, false
local frames, drivers, overrides, units = {}, {}, {}, {}
local bindings = {PADRSTICK="INTERACTTARGET", ["SHIFT-PADRSTICK"]="INTERACTTARGET"}
local claims = {}
ConsolePortLayers = {RunAttribute=function(_,method,owner,...)
    assert(secure and owner=='FlareUIInteractionDriver')
    if method=='Release' then claims[(...)]=nil
    else
        local level,key,kind,button,click=...
        assert(method=='Claim' and level=='OVERRIDE' and kind=='click' and click=='LeftButton')
        claims[key]=button
    end
end}
local attacks, interactions, shots = 0, {}, 0
UIParent = {}
function InCombatLockdown() return combat end
function UnitClass() return "Hunter", "HUNTER" end
C_Spell = {GetSpellName=function(id) assert(id==75);return "Auto Shot" end}
function UnitExists(unit) return units[unit]~=nil end
function UnitIsUnit(a,b)
    assert(not combat, 'never inspect unit identity in combat')
    if identityRestricted then error('secret unit identity') end
    return units[a]~=nil and units[a]==units[b]
end
function GetBindingAction(key) return bindings[key] end
function RunBinding() error('cannot invoke native interaction from addon Lua') end
C_PlayerInteractionManager = {InteractUnit=RunBinding}
local managerEvents = {}
SecureStateDriverManager = {RegisterEvent=function(_,event) managerEvents[event]=true end}

local function restricted(fn, ...)
    local previous=secure;secure=true
    fn(...)
    secure=previous
end
function CreateFrame(kind,name,parent,template)
    assert(not combat)
    local frame={attributes={},events={},refs={}}
    function frame:SetFrameRef(key,value) assert(not combat);self.refs[key]=value end
    function frame:GetFrameRef(key) return self.refs[key] end
    function frame:RegisterForClicks(...) self.clicks={...} end
    function frame:RegisterEvent(event) self.events[event]=true end
    function frame:SetScript(kind,fn) self[kind]=fn end
    function frame:GetAttribute(key) return self.attributes[key] end
    function frame:SetAttribute(key,value)
        assert(not combat or secure, 'insecure protected attribute mutation in combat')
        if self.attributes[key]==value then return end
        self.attributes[key]=value
        local snippet=self.attributes._onattributechanged
        if snippet then
            local fn=assert(loadstring('return function(self,name,value) '..snippet..' end'))()
            restricted(fn,self,key,value)
        end
    end
    function frame:ClearBindings() assert(secure);overrides={} end
    function frame:SetBindingClick(priority,key,button,click)
        assert(secure and priority==false and click=='LeftButton')
        overrides[key]=button
    end
    if kind=='Button' then
        assert(template=='SecureActionButtonTemplate' and parent==UIParent)
    elseif template then
        assert(template=='SecureHandlerAttributeTemplate')
    end
    frames[#frames+1]=frame
    if name then frames[name]=frame end
    return frame
end

-- Evaluate the small, documented macro-condition subset used by the actual
-- drivers and macros. Actions are simulated; protected execution needs WoW.
local function trim(s) return s:match('^%s*(.-)%s*$') end
local function conditions(text)
    local unit=text:match('@([%w]+)') or 'target'
    for term in text:gmatch('[^,]+') do
        if term:sub(1,1)~='@' then
            local invert=term:sub(1,2)=='no'
            local test=invert and term:sub(3) or term
            local value
            if test=='combat' then value=combat
            elseif test=='exists' then value=UnitExists(unit)
            elseif test=='harm' or test=='dead' then value=units[unit] and units[unit][test] or false
            else error('unhandled condition: '..term) end
            if (not not value)==invert then return false,unit end
        end
    end
    return true,unit
end
local function parse(text)
    for clause in (text..';'):gmatch('(.-);') do
        clause=trim(clause)
        if clause:sub(1,1)~='[' then return clause,'target' end
        local value=clause:match('.*%](.*)$')
        for group in clause:gmatch('%[(.-)%]') do
            local match,unit=conditions(group)
            if match then return trim(value),unit end
        end
    end
end
function RegisterAttributeDriver(frame,attribute,condition)
    assert(not combat)
    drivers[#drivers+1]={frame=frame,attribute=attribute,condition=condition}
    restricted(frame.SetAttribute,frame,attribute,parse(condition))
end
local function event(name)
    for _,driver in ipairs(drivers) do
        restricted(driver.frame.SetAttribute,driver.frame,driver.attribute,parse(driver.condition))
    end
    for _,frame in ipairs(frames) do
        if frame.events[name] then frame:OnEvent(name) end
    end
end
local function press(key)
    key=key or 'PADRSTICK'
    local name=claims[key] or overrides[key]
    local button=name and frames[name]
    if not button then
        assert(bindings[key]=='INTERACTTARGET')
        interactions[#interactions+1]=units.target or units.softinteract or 'none'
        return
    end
    assert(button.attributes.type=='macro' and button.attributes.useOnKeyDown)
    for line in button.attributes.macrotext:gmatch('[^\n]+') do
        local command,text=line:match('^/(%S+)%s*(.*)$')
        local value,unit=parse(text)
        if command=='stopmacro' then if value then return end
        elseif value then
            if command=='target' then units.target=units[unit] or units.target
            elseif command=='cleartarget' then units.target=nil
            elseif command=='startattack' then attacks=attacks+1
            elseif command=='cast' then assert(value=='!Auto Shot');shots=shots+1
            else error('unhandled macro action: '..command) end
        end
    end
end

local ns={}
assert(loadfile(source))('FlareUI',ns)
ns.ApplyControllerTargeting('PADRSTICK')
local enemy,enemy2,friend,npc,object={harm=true},{harm=true},{},{},{}
units.softenemy,units.softfriend,units.softinteract=enemy,friend,npc
event('PLAYER_SOFT_ENEMY_CHANGED');press()
assert(units.target==enemy and attacks==1 and shots==1 and #interactions==0,'one press selects and attacks the enemy ahead of a friendly')
units.softenemy=enemy2;event('PLAYER_SOFT_ENEMY_CHANGED');press()
assert(units.target==enemy2 and attacks==2,'switch away from the previous hard target')
assert(claims.PADRSTICK=='FlareUISoftEnemy' and not overrides.PADRSTICK,'use ConsolePort claims instead of competing with its base trigger rows')
units.softenemy=nil
press();assert(attacks==2,'a stale enemy override must not attack the old target')
event('PLAYER_SOFT_ENEMY_CHANGED');press()
assert(units.target==friend and attacks==2,'select a non-interactable friendly without attacking')
event('PLAYER_TARGET_CHANGED');press()
assert(interactions[#interactions]==friend,'the same key now uses native interaction')

units.target=nil;units.softfriend=npc;units.softinteract=npc
event('PLAYER_SOFT_FRIEND_CHANGED');press()
assert(interactions[#interactions]==npc and not overrides.PADRSTICK,'unobstructed NPC goes straight to native interaction')
units.target=enemy;event('PLAYER_TARGET_CHANGED');press()
assert(units.target==npc,'first press switches from an unrelated hard target to the NPC')
event('PLAYER_TARGET_CHANGED');press()
assert(interactions[#interactions]==npc,'second press interacts with the selected NPC')
units.softfriend=nil;units.softinteract=object
event('PLAYER_SOFT_INTERACT_CHANGED');press()
assert(units.target==nil,'clear a hard target that would steal object interaction')
event('PLAYER_TARGET_CHANGED');press()
assert(interactions[#interactions]==object,'next press uses the object')

units.target=enemy;units.softfriend=npc;units.softinteract=npc
combat=true;event('PLAYER_REGEN_DISABLED');press()
assert(units.target==nil,'combat uses secure clearing rather than stale unit identity')
event('PLAYER_TARGET_CHANGED');press()
assert(interactions[#interactions]==npc,'combat interaction works on the next press without a modifier')
units.softenemy=enemy2;event('PLAYER_SOFT_ENEMY_CHANGED');press()
assert(units.target==enemy2 and attacks==3,'enemy selection and attack work in combat')
local before=attacks;press('SHIFT-PADRSTICK')
assert(attacks==before,'modified R3 remains native interaction')
combat=false;event('PLAYER_REGEN_ENABLED')

identityRestricted=true;units.softenemy=nil;event('PLAYER_SOFT_FRIEND_CHANGED')
press();assert(units.target==npc,'restricted identity fails safely to explicit selection')
identityRestricted=false;event('PLAYER_TARGET_CHANGED')
ns.ApplyControllerTargeting(nil);assert(not next(overrides) and not next(claims),'profile opt-out releases bindings')
ns.ApplyControllerTargeting('PADRSTICK')
bindings.PADRSTICK='TOGGLEAUTORUN';event('UPDATE_BINDINGS')
assert(not next(overrides) and not next(claims),'an in-game rebind must not be overridden')
assert(managerEvents.PLAYER_SOFT_ENEMY_CHANGED and managerEvents.PLAYER_SOFT_FRIEND_CHANGED and managerEvents.PLAYER_SOFT_INTERACT_CHANGED)
print('PASS: enemy priority, selection/attack, native NPC/object interaction, combat transitions, stale candidates, modifiers and user rebinding')

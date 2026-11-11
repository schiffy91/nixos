local _, ns = ...
local driver, configuredKey

local function MacroButton(name, text)
    local button = CreateFrame("Button", name, UIParent, "SecureActionButtonTemplate")
    button:RegisterForClicks("AnyDown", "AnyUp")
    button:SetAttribute("useOnKeyDown", true)
    button:SetAttribute("type", "macro")
    button:SetAttribute("macrotext", text)
    return button
end

local function Initialize()
    local attack = "/stopmacro [@softenemy,noexists][@softenemy,noharm][@softenemy,dead]\n/target [@softenemy]\n/startattack [harm,nodead]"
    if select(2, UnitClass("player")) == "HUNTER" then
        local spell = C_Spell.GetSpellName(75)
        if spell then attack = attack .. "\n/cast [harm,nodead] !" .. spell end
    end
    MacroButton("FlareUISoftEnemy", attack)
    MacroButton("FlareUISoftFriend", "/target [@softfriend,exists]")
    MacroButton("FlareUIClearInteractionTarget", "/cleartarget [@softfriend,exists][@softinteract,exists]")

    driver = CreateFrame("Frame", "FlareUIInteractionDriver", UIParent, "SecureHandlerAttributeTemplate")
    if ConsolePortLayers then driver:SetFrameRef("layers", ConsolePortLayers) end
    driver:SetAttribute("_onattributechanged", [[
        if name ~= "key" and name ~= "same" and name ~= "interactable" and name ~= "state-candidate"
            and name ~= "state-locked" and name ~= "state-combat" then return end
        self:ClearBindings()
        local layers = self:GetFrameRef("layers")
        local previous = self:GetAttribute("applied-key")
        if layers and previous then layers:RunAttribute("Release", "FlareUIInteractionDriver", previous) end
        local key = self:GetAttribute("key")
        self:SetAttribute("applied-key", key)
        if not key then return end
        local candidate = self:GetAttribute("state-candidate")
        local locked = self:GetAttribute("state-locked") == "yes"
        local combat = self:GetAttribute("state-combat") == "yes"
        local button
        if candidate == "enemy" then
            button = "FlareUISoftEnemy"
        elseif candidate == "friend" then
            if combat then
                if locked then button = "FlareUIClearInteractionTarget" end
            elseif not self:GetAttribute("same") and (locked or not self:GetAttribute("interactable")) then
                button = "FlareUISoftFriend"
            end
        elseif candidate == "interact" and locked and (combat or not self:GetAttribute("same")) then
            button = "FlareUIClearInteractionTarget"
        end
        -- Releasing the override returns this key to Blizzard's INTERACTTARGET.
        -- ConsolePort owns the base modifier rows. Claim above those rows but
        -- below navigation so changing triggers cannot steal this action.
        if button then
            if layers then
                layers:RunAttribute("Claim", "FlareUIInteractionDriver", "OVERRIDE", key, "click", button, "LeftButton")
            else
                self:SetBindingClick(false, key, button, "LeftButton")
            end
        end
    ]])
    RegisterAttributeDriver(driver, "state-candidate", "[@softenemy,harm,nodead] enemy; [@softfriend,exists] friend; [@softinteract,exists] interact; none")
    RegisterAttributeDriver(driver, "state-locked", "[@target,exists] yes; no")
    RegisterAttributeDriver(driver, "state-combat", "[combat] yes; no")
    for _, event in ipairs({"PLAYER_SOFT_ENEMY_CHANGED", "PLAYER_SOFT_FRIEND_CHANGED", "PLAYER_SOFT_INTERACT_CHANGED"}) do
        SecureStateDriverManager:RegisterEvent(event)
    end
end

local function Refresh()
    if not driver or InCombatLockdown() then return end
    -- Unit identity can be restricted in combat. The secure driver clears a
    -- blocking target there, then lets the next press use native interaction.
    local ok, same = pcall(function()
        local unit = UnitExists("softfriend") and "softfriend" or "softinteract"
        return UnitIsUnit(unit, "target") and true or false
    end)
    driver:SetAttribute("same", ok and same or false)
    local interactionOK, interactable = pcall(function()
        return UnitIsUnit("softfriend", "softinteract") and true or false
    end)
    driver:SetAttribute("interactable", interactionOK and interactable or false)
    local key = configuredKey
    driver:SetAttribute("key", key and GetBindingAction(key) == "INTERACTTARGET" and key or nil)
end

function ns.ApplyControllerTargeting(key)
    if not driver and not key then return end
    if not driver then Initialize() end
    configuredKey = key
    Refresh()
end

local events = CreateFrame("Frame")
for _, event in ipairs({"UPDATE_BINDINGS", "PLAYER_TARGET_CHANGED", "PLAYER_SOFT_FRIEND_CHANGED", "PLAYER_SOFT_INTERACT_CHANGED", "PLAYER_REGEN_ENABLED"}) do
    events:RegisterEvent(event)
end
events:SetScript("OnEvent", Refresh)

local _, ns = ...
local editMode = LibStub("FlareEditMode")
local pending
local appliedProfile
local screenWidth, screenHeight
local loggedIn = false
local fittingToolbar = false

local function Settings()
    local settings = ns.db and ns.db.profile.displayLayout
    return settings and settings.enabled and settings
end

local function ApplyChat(settings)
    if not (ChatFrame1 and settings) then return end
    ChatFrame1:ClearAllPoints()
    ChatFrame1:SetPoint("BOTTOMLEFT", UIParent, "BOTTOMLEFT", settings.x, settings.y)
    ChatFrame1:SetSize(settings.width, settings.height)
    FCF_SavePositionAndDimensions(ChatFrame1)
end

local function FitToolbar()
    local settings = Settings()
    local fit = settings and settings.toolbar
    local toolbar, watchbar = ConsolePortToolbar, ConsolePortWatchbar
    if fittingToolbar or InCombatLockdown() or not fit or not toolbar or not watchbar or not watchbar.XPBar then return end
    local menu, grid, xp = toolbar.Menu, toolbar.Menu.Grid, watchbar.XPBar
    local children = grid:GetLayoutChildren()
    local naturalWidth, naturalHeight = 0, 0
    for _, child in ipairs(children) do
        naturalWidth = naturalWidth + child:GetWidth()
        naturalHeight = math.max(naturalHeight, child:GetHeight())
    end
    if naturalWidth <= 0 or naturalHeight <= 0 then return end
    local unit = toolbar:GetEffectiveScale()
    local uiUnits = UIParent:GetEffectiveScale() / unit
    local left, right = CPB_PADLTRIGGER, CPB_PADRTRIGGER
    if not left or not right or not left:GetBottom() or not right:GetBottom() or not watchbar:GetBottom() then return end
    local gaps = math.max(0, #children - 1)
    local padding = fit.padding or 2
    local endCaps = xp:GetEndCapWidth() * 2
    -- Size the visible row first. The invisible menu frame and XP container
    -- both include spare space that should not count toward alignment.
    local widthLimit = (watchbar:GetWidth() * .8 - endCaps) * xp:GetEffectiveScale() / unit
    local scale = math.min(widthLimit / (naturalWidth + gaps * padding),
        ((fit.height or 44) - 2) * uiUnits / naturalHeight, (fit.iconWidth or 28) * uiUnits / 32)
    local width = (naturalWidth + gaps * padding) * scale
    local height = naturalHeight * scale + 2 * uiUnits
    local stripWidth = width * unit / xp:GetEffectiveScale() + endCaps
    local stripHeight = fit.xpHeight and fit.xpHeight * UIParent:GetEffectiveScale() / xp:GetEffectiveScale()
    if math.abs(xp:GetWidth() - stripWidth) > .001 or (stripHeight and xp.barHeight ~= stripHeight) then
        fittingToolbar = true
        if stripHeight then xp.barHeight = stripHeight end
        xp:SetWidth(stripWidth)
        xp:UpdateBarsShown()
        fittingToolbar = false
    end
    for _, bar in ipairs(xp.bars or {}) do
        if bar.ExhaustionLevelFillBar then
            -- ConsolePort's XML fixes this separate rested texture at 14.
            bar.ExhaustionLevelFillBar:SetHeight(bar.StatusBar:GetHeight())
        end
        -- Rested XP is on the parent frame's background layer. Keep the empty
        -- track underneath it, rather than dimming it with a child backdrop.
        bar.StatusBar.Background:Hide()
        if not bar.displayLayoutTrack then
            bar.displayLayoutTrack = bar:CreateTexture(nil, "BACKGROUND", nil, -2)
        end
        local track = bar.displayLayoutTrack
        track:SetAllPoints(bar.StatusBar)
        track:SetColorTexture(.16, .13, .08, .65)
        track:Show()
    end
    -- The watch-container gradients and menu nine-slice otherwise leave a
    -- black strip behind this compact stack; retain individual button borders.
    xp.BGLeft:Hide()
    xp.BGRight:Hide()
    menu.Border:SetAlpha(0)
    local triggerBottom = math.min(left:GetBottom() * left:GetEffectiveScale(), right:GetBottom() * right:GetEffectiveScale()) / unit
    -- ConsolePort anchors the actual status bars to Watchbar, not XPBar.
    -- Measure their visible border textures, including a second reputation bar.
    local low, high
    for _, key in ipairs({"SingleBarSmall", "SingleBarLarge", "SingleBarSmallUpper", "SingleBarLargeUpper"}) do
        local texture = xp[key]
        if texture and texture:IsShown() and texture:GetBottom() and texture:GetTop() then
            local scale = xp:GetEffectiveScale() / unit
            low = math.min(low or math.huge, texture:GetBottom() * scale)
            high = math.max(high or -math.huge, texture:GetTop() * scale)
        end
    end
    local watchBottom = watchbar:GetBottom() * watchbar:GetEffectiveScale() / unit
    low, high = low or watchBottom, high or (watchBottom + xp:GetHeight() * xp:GetEffectiveScale() / unit)
    local xpHeight = high - low
    local bottom = (fit.bottom or 0) * uiUnits
    local deficit = bottom + height + xpHeight + 2 * (fit.minGap or 4) * uiUnits - triggerBottom
    if deficit > 0 then
        local cluster = ConsolePortBarCluster
        if not cluster then return end
        local point, relative, relativePoint, x, y = cluster:GetPoint()
        if not point then return end
        cluster:SetPoint(point, relative, relativePoint, x, y + deficit * unit / cluster:GetEffectiveScale())
        triggerBottom = triggerBottom + deficit
    end
    local gap = (triggerBottom - bottom - height - xpHeight) / 2
    fittingToolbar = true
    menu:SetScale(1)
    grid.childXPadding = padding
    grid:SetScale(scale)
    grid:Layout()
    menu:SetSize(width, height)
    menu:ClearAllPoints()
    menu:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, bottom)
    local watchY = bottom + height + gap - (low - watchBottom)
    watchbar:ClearAllPoints()
    watchbar:SetPoint("BOTTOM", UIParent, "BOTTOM", 0, watchY * unit / watchbar:GetEffectiveScale())
    -- ConsolePort offsets this border downward; align it to the fitted menu so
    -- clearing the XP bar does not require lifting the whole row.
    menu.Border:ClearAllPoints()
    menu.Border:SetPoint("TOPLEFT", menu, "TOPLEFT", 0, 0)
    menu.Border:SetPoint("BOTTOMRIGHT", menu, "BOTTOMRIGHT", 0, 0)
    toolbar.Notch:SetHoverZone(width, height)
    toolbar.Notch:ClearAllPoints()
    toolbar.Notch:SetPoint("CENTER", menu, "CENTER")
    fittingToolbar = false
end

local function PrepareToolbar()
    local toolbar = ConsolePortToolbar
    if not toolbar or toolbar.displayLayoutHooked then return end
    toolbar.displayLayoutHooked = true
    hooksecurefunc(toolbar, "OnGridSizeChanged", FitToolbar)
    hooksecurefunc(toolbar, "UpdateDirection", FitToolbar)
    toolbar.Menu:HookScript("OnShow", FitToolbar)
    hooksecurefunc(ConsolePortWatchbar, "SetProps", FitToolbar)
    hooksecurefunc(ConsolePortWatchbar.XPBar, "LayoutBars", FitToolbar)
    if ConsolePortBarCluster then hooksecurefunc(ConsolePortBarCluster, "SetProps", FitToolbar) end
    FitToolbar()
end

local tooltipHooked = false
local function PrepareTooltip()
    if tooltipHooked then return end
    tooltipHooked = true
    hooksecurefunc("GameTooltip_SetDefaultAnchor", function(tooltip, parent)
        local settings = Settings()
        local anchor = settings and settings.tooltip
        if not anchor or tooltip ~= GameTooltip or tooltip:IsForbidden() or not MinimapCluster then return end
        tooltip:SetOwner(parent, "ANCHOR_NONE")
        tooltip:ClearAllPoints()
        tooltip:SetPoint("BOTTOMRIGHT", MinimapCluster, "BOTTOMLEFT", -(anchor.gap or 12), anchor.y or 0)
    end)
end

local softTargetButton
local function PrepareSoftTarget(driver)
    if not driver and not softTargetButton then return end
    if not softTargetButton then
        softTargetButton = CreateFrame("Button", "FlareUISoftTarget", UIParent, "SecureActionButtonTemplate")
        softTargetButton:RegisterForClicks("AnyDown", "AnyUp")
        softTargetButton:SetAttribute("useOnKeyDown", true)
        softTargetButton:SetAttribute("type", "target")
        _G["BINDING_NAME_CLICK FlareUISoftTarget:LeftButton"] = "Select Soft Target (Enemy First)"
    end
    -- The secure driver can change units during combat. Selecting never starts
    -- an attack or interacts; modified R3 retains the native Interact binding.
    if driver then
        RegisterAttributeDriver(softTargetButton, "unit", driver)
    else
        UnregisterAttributeDriver(softTargetButton, "unit")
        softTargetButton:SetAttribute("unit", nil)
    end
end

local function ApplyProfile()
    if InCombatLockdown() or editMode:IsInEditMode() then pending = true; return end
    pending = nil
    local settings = Settings()
    PrepareSoftTarget(settings and settings.softTargetUnit)
    if not settings then return end
    for name, value in pairs(settings.cvars or {}) do
        if GetCVar(name) ~= nil and GetCVar(name) ~= value then SetCVar(name, value) end
    end
    local global = ns.db.global
    local changed = global.uiScale ~= "custom" or global.uiScaleCustom ~= settings.scale
    global.uiScale, global.uiScaleCustom = "custom", settings.scale
    local controller = settings.input == "controller"
    -- Forever keys character-specific addon state by GUID, like Blizzard's
    -- AddonList. UnitName returns only the first name in this client.
    local character = UnitGUID("player")
    if not character then pending = true; return end
    if controller then C_AddOns.EnableAddOn("ConsolePort_Bar", character)
    else C_AddOns.DisableAddOn("ConsolePort_Bar", character) end
    if C_AddOns.IsAddOnLoaded("ConsolePort_Bar") ~= controller then changed = true end
    if appliedProfile and appliedProfile ~= ns.db:GetCurrentProfile() then changed = true end
    if changed then
        ns.ShowDialog("FLAREUI_RELOAD")
    else
        appliedProfile = ns.db:GetCurrentProfile()
        ApplyChat(settings.chat)
        PrepareTooltip()
        PrepareToolbar()
    end
end

local function RememberDisplay(width, height)
    screenWidth, screenHeight = width, height
    ns.db.global.displayLayoutScreen = {width = width, height = height}
end

local function ApplyChosenProfile()
    local width, height = GetPhysicalScreenSize()
    if width and height and width > 0 and height > 0 then RememberDisplay(width, height) end
    ApplyProfile()
end

-- Persist the observed resolution so a reload is not mistaken for a display
-- change. Explicit profile choices survive reloads and restarts at that size.
-- Native Edit Mode selection and saving belong
-- to Blizzard; calling SaveLayouts here can crash the Forever client.
local function FollowDisplay()
    local settings = Settings()
    if not settings then return end
    if InCombatLockdown() or editMode:IsInEditMode() then pending = true; return end
    local width, height = GetPhysicalScreenSize()
    if not width or not height or width <= 0 or height <= 0 then return end
    local previous = ns.db.global.displayLayoutScreen
    local changed = previous and (previous.width ~= width or previous.height ~= height)
    -- Existing profiles keep their selection on the first run of this code.
    -- A fresh character using Default still gets an appropriate initial layout.
    if not previous then changed = ns.db:GetCurrentProfile() == "Default" end
    RememberDisplay(width, height)
    if not changed then ApplyProfile(); return end
    local display = height <= 1000 and "Handheld" or height >= 2800 and "Desk" or "TV"
    local name = display .. " / " .. (settings.input == "controller" and "Controller" or "Keyboard")
    local profile = ns.db.sv.profiles[name]
    if profile and profile.displayLayout and profile.displayLayout.enabled and ns.db:GetCurrentProfile() ~= name then
        ns.db:SetProfile(name)
    else
        ApplyProfile()
    end
end

editMode:RegisterCallback("exit", function()
    local settings = Settings()
    if settings and appliedProfile == ns.db:GetCurrentProfile() and ChatFrame1 then
        local scale = ChatFrame1:GetEffectiveScale() / UIParent:GetEffectiveScale()
        local x, y, width, height = ChatFrame1:GetRect()
        if x and y and width and height then
            settings.chat = { x = x * scale, y = y * scale,
                width = width * scale, height = height * scale }
        end
    end
    if pending then C_Timer.After(0, FollowDisplay) end
end)

local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("PLAYER_REGEN_ENABLED")
frame:RegisterEvent("DISPLAY_SIZE_CHANGED")
frame:SetScript("OnEvent", function(_, event)
    if not ns.db then return end
    if event == "PLAYER_LOGIN" then
        loggedIn = true
        ns.db.RegisterCallback(frame, "OnProfileChanged", ApplyChosenProfile)
        ns.db.RegisterCallback(frame, "OnProfileCopied", ApplyChosenProfile)
        ns.db.RegisterCallback(frame, "OnProfileReset", ApplyChosenProfile)
        C_Timer.After(0, FollowDisplay)
    elseif not loggedIn then
        return
    elseif event == "DISPLAY_SIZE_CHANGED" then
        local width, height = GetPhysicalScreenSize()
        if width ~= screenWidth or height ~= screenHeight then C_Timer.After(0, FollowDisplay) end
    elseif pending then
        C_Timer.After(0, FollowDisplay)
    elseif event == "PLAYER_REGEN_ENABLED" then
        FitToolbar()
    end
end)

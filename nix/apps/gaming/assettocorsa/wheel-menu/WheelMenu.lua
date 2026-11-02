local Menu = require('Menu')
local menu = Menu.new()
local previous = ac.ControlButton('__NIX_MFD_PREVIOUS')
local nextItem = ac.ControlButton('__NIX_MFD_NEXT')
local decrease = ac.ControlButton('__NIX_MFD_DECREASE')
local increase = ac.ControlButton('__NIX_MFD_INCREASE')
local toggle = ac.ControlButton('__NIX_MFD_TOGGLE')
local back = ac.ControlButton('HANDBRAKE'):setAlwaysActive(true)
local cancelOnRelease
local heldCommand
local lastError

local entries = require('CarMenu')

function script.update(dt)
  local sim, car = ac.getSim(), ac.getCar(0)
  -- Cancel only a pause that was already open when Circle was pressed. Waiting
  -- for release prevents the shared handbrake binding applying on resume.
  if back:pressed() then
    cancelOnRelease = nil
    if sim.isReplayActive and not sim.isReplayOnlyMode then cancelOnRelease = 'replay'
    elseif sim.isPaused and not sim.isInMainMenu then cancelOnRelease = 'pause' end
  end
  if back:released() then
    if cancelOnRelease == 'replay' and sim.isReplayActive and not sim.isReplayOnlyMode then
      ac.tryToToggleReplay(false)
    elseif cancelOnRelease == 'pause' and sim.isPaused and not sim.isInMainMenu and not sim.isReplayActive then
      ac.tryToPause(false)
    end
    cancelOnRelease = nil
  end
  local selected = menu.selected
  local ok, values = pcall(entries, car, sim)
  if not ok then
    if values ~= lastError then ac.error(values) end
    lastError, heldCommand = values, nil
    menu:replace({{id = 'error', label = 'Wheel controls unavailable', value = 'See CSP log'}})
    menu.visible = true
    return
  end
  lastError = nil
  menu:replace(values)
  if selected and selected ~= menu.selected then
    heldCommand = nil
    return -- do not apply an input to a different control as capabilities change
  end
  if sim.isInMainMenu or sim.isPaused or sim.isReplayActive or not sim.isLive then
    heldCommand = nil
    return
  end
  if previous:pressed() then menu:select(-1) end
  if nextItem:pressed() then menu:select(1) end
  if decrease:pressed() then menu:adjust(-1, true) end
  if increase:pressed() then menu:adjust(1, true) end
  if toggle:pressed() then
    local entry = menu:current()
    if not menu.visible then menu.visible = true
    elseif entry and entry.hold then heldCommand = entry.id
    elseif entry and entry.activate then menu:activate(true)
    else menu.visible = false end
  end
  local entry = menu:current()
  if not toggle:down() or not entry or entry.id ~= heldCommand then heldCommand = nil end
  if heldCommand then entry.hold() end
end

function script.drawMenu()
  local sim = ac.getSim()
  if not menu.visible or sim.isInMainMenu or sim.isPaused or sim.isReplayActive then return end
  local entry, index = menu:current()
  if not entry then return end
  -- CSP already applies the shared UI scale to these logical coordinates.
  local screen = ui.windowSize()
  local origin = vec2(math.max(16, screen.x - 380), math.max(16, screen.y - 192))
  ui.drawRectFilled(origin, origin + vec2(360, 172), rgbm(0.04, 0.05, 0.06, 0.94), 8)
  ui.dwriteDrawText(string.format('WHEEL MENU   %d / %d', index, #menu.entries), 15,
    origin + vec2(18, 12), rgbm(0.65, 0.7, 0.75, 1))
  ui.dwriteDrawText(entry.label, 22, origin + vec2(18, 40), rgbm.colors.white)
  ui.dwriteDrawText(entry.format and entry.format(entry.value) or tostring(entry.value), 30,
    origin + vec2(18, 70), rgbm.colors.white)
  local hint = entry.hint or ((entry.set or entry.adjust) and 'Right dial: adjust' or 'Read only')
  if entry.readOnly then hint = 'Locked by game' end
  ui.dwriteDrawText('Left dial: select', 14,
    origin + vec2(18, 120), rgbm(0.65, 0.7, 0.75, 1))
  ui.dwriteDrawText(hint, 14,
    origin + vec2(18, 141), rgbm(0.65, 0.7, 0.75, 1))
end

ac.log('Wheel Menu loaded')

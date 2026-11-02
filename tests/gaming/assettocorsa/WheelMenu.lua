package.path = arg[1] .. '/?.lua;' .. package.path
local buttons, writes = {}, {}
local sim = {isLive = true, firstPersonCameraFOV = 56}
local car = {physicsAvailable = true, tractionControlModes = 3, tractionControlMode = 1,
  tractionControl2Modes = 0, fuelMaps = 0, absModes = 0, turboCount = 0, wiperModes = 1,
  fuel = 30, fuelPerLap = 3, ffbMultiplier = 1, speedKmh = 20, gear = 1, rpm = 1000}
local interface = {appsHidden = false, currentDesktop = 0}
local errors = {}
ac = {
  CSPModuleID = {General = 'general'},
  INIConfig = {cspModule = function() return {get = function() return true end} end},
  getSim = function() return sim end,
  getCar = function() return car end,
  getUI = function() return interface end,
  switchToNeutralGear = function() car.gear = 0 end,
  setAppsHidden = function(value) interface.appsHidden = value end,
  setCurrentDesktop = function(value) interface.currentDesktop = value end,
  getPitstopSpinners = function() return {} end,
  setTC = function(value) writes[#writes + 1] = value end,
  log = function() end,
  error = function(message) errors[#errors + 1] = message end,
  tryToPause = function(active) sim.isPaused = active end,
  tryToToggleReplay = function(active) sim.isReplayActive = active end,
  ControlButton = function(id)
    local button = {press = false, release = false}
    function button:pressed() return self.press end
    function button:released() return self.release end
    function button:down() return self.held end
    function button:setAlwaysActive() return self end
    buttons[id] = button
    return button
  end,
}
script = {}
dofile(arg[1] .. '/WheelMenu.lua')
local function frame(pressed, released, held)
  for id, button in pairs(buttons) do
    button.press, button.release = id == pressed, id == released
    button.held = id == pressed or id == held
  end
  script.update(0.016)
end
frame('__NIX_MFD_INCREASE')
assert(writes[1] == 2)
sim.isPaused = true
frame('__NIX_MFD_INCREASE')
assert(#writes == 1)
frame('HANDBRAKE')
assert(sim.isPaused) -- do not resume with the handbrake still held
frame(nil, 'HANDBRAKE')
assert(not sim.isPaused)
sim.isReplayActive = true
frame('__NIX_MFD_INCREASE')
assert(#writes == 1)
frame('HANDBRAKE')
assert(sim.isReplayActive)
frame(nil, 'HANDBRAKE')
assert(not sim.isReplayActive)
sim.isReplayActive, sim.isReplayOnlyMode = true, true
frame('HANDBRAKE')
frame(nil, 'HANDBRAKE')
assert(sim.isReplayActive) -- standalone replay has no driving session to resume
sim.isReplayOnlyMode = false
sim.isReplayActive, sim.isInMainMenu = false, true
frame('__NIX_MFD_INCREASE')
assert(#writes == 1)
sim.isInMainMenu = false
frame('HANDBRAKE') -- driving: no cancellation armed
sim.isPaused = true
frame(nil, 'HANDBRAKE')
assert(sim.isPaused)
sim.isPaused = false
frame('__NIX_MFD_NEXT') -- read-only fuel page
frame('__NIX_MFD_INCREASE')
assert(#writes == 1)
print('PASS: CSP input dispatch, pause/replay/menu isolation and release-before-resume')

-- Exercise actual adapter selection and rendering with a car exposing the
-- optional systems. Setters update the fake car just as the next physics frame
-- would, so assertions observe acknowledged values, not optimistic UI state.
local drawn = {}
local vector = {__add = function(a, b) return vec2(a.x + b.x, a.y + b.y) end}
function vec2(x, y) return setmetatable({x = x, y = y}, vector) end
rgbm = setmetatable({colors = {white = {}}}, {__call = function() return {} end})
ui = {
  windowSize = function() return vec2(1920, 1080) end,
  drawRectFilled = function() end,
  dwriteDrawText = function(value) drawn[#drawn + 1] = value end,
}
local function display()
  drawn = {}
  script.drawMenu()
  return drawn[2], drawn[3], drawn[5]
end
local function select(label)
  for _ = 1, 100 do
    if display() == label then return end
    frame('__NIX_MFD_NEXT')
  end
  error('Unreachable wheel menu entry: ' .. label)
end
ac.TurningLights = {None = 0, Left = 1, Right = 2, Hazards = 3}
car.tractionControl2Modes, car.tractionControl2 = 3, 1
car.fuelMaps, car.fuelMap, car.hShifter = 3, 0, true
sim.controlsWithShifter = true
ac.setHShifterActive = function(value) sim.controlsWithShifter = value end
car.hasLowBeams, car.lowBeams, car.hasTurningLights = true, true, true
car.hasCockpitMGUHMode, car.mguhChargingBatteries = true, false
car.hasCockpitERSDelivery, car.mgukDeliveryCount, car.mgukDelivery = true, 2, 0
car.wiperModes, car.wiperSelectedMode = 4, 0
ac.setWiperMode = function(value) car.wiperSelectedMode = value end
ac.setHighBeams = function(value) car.lowBeams = not value end
ac.setTurningLights = function(value)
  car.hazardLights, car.turningLeftOnly, car.turningRightOnly = value == 3, value == 1, value == 2
end
ac.setMGUHCharging = function(value) car.mguhChargingBatteries = value end
ac.setMGUKDelivery = function(value) car.mgukDelivery = value end
ac.getMGUKDeliveryName = function(_, index) return ({'Balanced', 'Overtake'})[index + 1] end
local commands = {}
ac.simulateCustomHotkeyPress = function(command)
  commands[#commands + 1] = command
  if command == '__EXT_ENGINEMAP_UP' then
    car.fuelMap = (car.fuelMap + 1) % car.fuelMaps
    return
  end
  if command == '__EXT_TC2_UP' or command == '__EXT_TC2_DOWN' then
    car.tractionControl2 = car.tractionControl2 + (command == '__EXT_TC2_UP' and 1 or -1)
  end
end
local spinners = {
  {name = 'Tyres', type = 'compound', value = -1, min = -1, max = 1},
  {name = 'Fuel', type = 'fuel', value = 20, min = 0, max = 100, readOnly = true},
  {name = 'Bodywork', type = 'repair', value = 0, min = 0, max = 1},
}
sim.currentQuickPitPreset = 0
ac.getPitstopSpinners = function() return spinners end
ac.setCurrentQuickPitPreset = function(value) sim.currentQuickPitPreset = value end
ac.setPitstopSpinnerValue = function(name, value)
  for _, spinner in ipairs(spinners) do
    if spinner.name == name then
      assert(not spinner.readOnly)
      spinner.value = value
      return true
    end
  end
  return false
end
ac.getTyresLongName = function(_, index) return ({'Soft', 'Medium'})[index + 1] end
frame()
select('Traction control · secondary')
frame('__NIX_MFD_INCREASE')
assert(car.tractionControl2 == 2)
frame('__NIX_MFD_DECREASE')
assert(car.tractionControl2 == 1)
select('Engine map')
frame('__NIX_MFD_DECREASE')
assert(car.fuelMap == 0) -- the single-direction command is explicit activation
frame('__NIX_MFD_TOGGLE')
assert(car.fuelMap == 1)
sim.isPaused = true
frame('__NIX_MFD_TOGGLE')
assert(car.fuelMap == 1)
sim.isPaused = false
select('Gear controls')
frame('__NIX_MFD_DECREASE')
assert(not sim.controlsWithShifter)
frame('__NIX_MFD_INCREASE')
assert(sim.controlsWithShifter)
select('Headlight beam')
frame('__NIX_MFD_INCREASE')
assert(not car.lowBeams)
select('Indicators')
frame('__NIX_MFD_INCREASE')
assert(car.turningLeftOnly)
frame('__NIX_MFD_DECREASE')
assert(not car.turningLeftOnly and not car.hazardLights)
select('Wipers')
frame('__NIX_MFD_INCREASE')
assert(car.wiperSelectedMode == 1)
select('MGU-H energy')
frame('__NIX_MFD_INCREASE')
assert(car.mguhChargingBatteries)
select('ERS delivery')
frame('__NIX_MFD_INCREASE')
frame()
local _, value = display()
assert(value == 'Overtake')
select('Pit strategy')
frame('__NIX_MFD_INCREASE')
assert(sim.currentQuickPitPreset == 1)
select('Pit · Tyres')
_, value = display()
assert(value == 'No tyre change')
frame('__NIX_MFD_INCREASE')
frame()
_, value = display()
assert(value == 'Soft')
select('Pit · Fuel')
frame('__NIX_MFD_INCREASE')
local _, fuel, hint = display()
assert(fuel == '20 L' and hint:find('Locked by game', 1, true))
select('Pit · Bodywork')
frame('__NIX_MFD_INCREASE')
assert(spinners[3].value == 1)
print('PASS: optional car controls, pit strategies, tyre labels, locked fuel and rendered feedback')

select('Gear · neutral')
frame('__NIX_MFD_TOGGLE')
assert(car.gear == 0)
select('Engine starter')
local before = #commands
frame('__NIX_MFD_TOGGLE')
frame(nil, nil, '__NIX_MFD_TOGGLE')
assert(#commands == before + 2 and commands[#commands] == '__EXT_STARTER')
frame(nil, '__NIX_MFD_TOGGLE')
assert(#commands == before + 2)
frame('__NIX_MFD_TOGGLE')
sim.isPaused = true
before = #commands
frame(nil, nil, '__NIX_MFD_TOGGLE')
sim.isPaused = false
frame(nil, nil, '__NIX_MFD_TOGGLE')
assert(#commands == before) -- resuming with the button held must not rearm
frame(nil, '__NIX_MFD_TOGGLE')
frame('__NIX_MFD_TOGGLE')
before = #commands
frame('__NIX_MFD_NEXT', nil, '__NIX_MFD_TOGGLE')
assert(#commands == before) -- changing selection cancels the held command
select('Engine cutoff')
frame('__NIX_MFD_TOGGLE')
assert(commands[#commands] == '__EXT_MANUAL_CUTOFF')
frame(nil, '__NIX_MFD_TOGGLE')
select('Force feedback')
frame('__NIX_MFD_DECREASE')
assert(commands[#commands] == '__EXT_FFB_DECREASE')
select('Driving apps')
frame('__NIX_MFD_DECREASE')
assert(interface.appsHidden)
frame('__NIX_MFD_INCREASE')
assert(not interface.appsHidden)
select('App page')
frame('__NIX_MFD_INCREASE')
assert(interface.currentDesktop == 1)

local seat = {position = {x = 0, y = 0, z = 0}, pitch = 0, yaw = 0}
ac.getOnboardCameraParams = function() return seat end
local function mirror(x, y, locked)
  return {rotation = {x = x, y = y}, fov = 10, notDrivenByUser = locked,
    clone = function(self) return mirror(self.rotation.x, self.rotation.y, self.notDrivenByUser) end}
end
local mirrors = {mirror(0, 0, false), mirror(0, 0, true)}
ac.getRealMirrorCount = function() return #mirrors end
ac.getRealMirrorParams = function(index) return mirrors[index + 1] end
ac.setRealMirrorParams = function(index, params, save)
  assert(save and params.fov == 10 and not mirrors[index + 1].notDrivenByUser)
  mirrors[index + 1] = params
end
car.speedKmh = 0
frame()
select('Seat · left / right')
frame('__NIX_MFD_INCREASE')
assert(commands[#commands] == '__EXT_SEAT_RIGHT')
select('Mirror 1 · horizontal')
frame('__NIX_MFD_INCREASE')
assert(mirrors[1].rotation.x == 0.01 and mirrors[1].rotation.y == 0)
select('Mirror 2 · vertical')
frame('__NIX_MFD_INCREASE')
assert(mirrors[2].rotation.y == 0)
select('Seat · left / right')
car.speedKmh = 20
before = #commands
local previousWrites = #writes
frame('__NIX_MFD_INCREASE')
assert(#commands == before and #writes == previousWrites and display() ~= 'Seat · left / right')
ac.INIConfig.cspModule = function() return {get = function() return false end} end
local withoutStalling = dofile(arg[1] .. '/CarMenu.lua')
for _, entry in ipairs(withoutStalling(car, sim)) do
  assert(entry.id ~= 'starter' and entry.id ~= 'cutoff')
end
car.speedKmh = 0
sim.firstPersonCameraFOV = nil
frame('__NIX_MFD_INCREASE')
assert(display() == 'Wheel controls unavailable' and #errors == 1)
frame()
assert(#errors == 1) -- do not flood the log each frame
sim.firstPersonCameraFOV = 56
frame()
select('Field of view')
_, value = display()
assert(value == '56.0°')
print('PASS: held engine controls, neutral, FFB/HUD dispatch and stationary cockpit/mirror editing')

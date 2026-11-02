local stalling = ac.INIConfig.cspModule(ac.CSPModuleID.General):get('PHYSICS_EXPERIMENTS', 'STALLING', false)

return function(car, sim)
  local result = {}
  local function add(id, label, value, minimum, maximum, step, setter, format)
    result[#result + 1] = {id = id, label = label, value = value, min = minimum,
      max = maximum, step = step, set = setter, format = format}
  end
  local function percent(value) return string.format('%.0f%%', value * 100) end
  local function choice(id, label, value, labels, setter)
    add(id, label, value, 0, #labels - 1, 1, setter,
      function(selected) return labels[selected + 1] or tostring(selected) end)
  end
  local function action(id, label, value, activate, hint)
    result[#result + 1] = {id = id, label = label, value = value, activate = activate, hint = hint}
  end
  local function command(id, label, value, down, up, hint)
    result[#result + 1] = {id = id, label = label, value = value, hint = hint,
      adjust = function(direction) ac.simulateCustomHotkeyPress(direction < 0 and down or up, 1) end}
  end
  if car.physicsAvailable then
    if car.tractionControlModes > 0 then
      add('tc', 'Traction control', car.tractionControlMode, 0, car.tractionControlModes, 1, ac.setTC)
    end
    if car.absModes > 0 then add('abs', 'ABS', car.absMode, 0, car.absModes, 1, ac.setABS) end
    if car.tractionControl2Modes > 0 then
      add('tc2', 'Traction control · secondary', car.tractionControl2, 0,
        car.tractionControl2Modes, 1, function(value)
          ac.simulateCustomHotkeyPress(value > car.tractionControl2 and '__EXT_TC2_UP' or '__EXT_TC2_DOWN')
        end)
    end
    if car.fuelMaps > 1 then
      -- CSP exposes a cycle command, not an absolute fuel-map setter. Label it
      -- as such rather than presenting a decrement that actually advances.
      action('engine-map', 'Engine map', car.fuelMap,
        function() ac.simulateCustomHotkeyPress('__EXT_ENGINEMAP_UP') end, 'Press right dial: next map')
    end
    if car.hasUserBrakeBias then
      add('brake-bias', 'Brake bias · front', car.brakeBias, 0, 1, 0.01, ac.setBrakeBias, percent)
    end
    for i = 0, car.turboCount - 1 do
      if ac.isTurboWastegateAdjustable(i) then
        local index = i
        add('turbo-' .. i, 'Turbo ' .. (i + 1), car.turboWastegates[i], 0, 1, 0.1,
          function(value) ac.setTurboWastegate(value, index) end, percent)
      end
    end
  end
  if car.hShifter then
    choice('shifter', 'Gear controls', sim.controlsWithShifter and 1 or 0, {'Paddles', 'H-pattern'},
      function(value) ac.setHShifterActive(value == 1) end)
  end
  action('neutral', 'Gear · neutral', car.gear == 0 and 'Neutral' or 'Select neutral',
    ac.switchToNeutralGear, 'Press right dial: neutral')
  if stalling then
    local function engine(id, label, key)
      result[#result + 1] = {id = id, label = label, value = string.format('%.0f RPM', car.rpm),
        hint = 'Hold right dial', hold = function() ac.simulateCustomHotkeyPress(key, 1) end}
    end
    engine('starter', 'Engine starter', '__EXT_STARTER')
    engine('cutoff', 'Engine cutoff', '__EXT_MANUAL_CUTOFF')
  end
  if car.wiperModes > 1 then
    add('wipers', 'Wipers', car.wiperSelectedMode, 0, car.wiperModes - 1, 1, ac.setWiperMode,
      function(value) return value == 0 and 'Off' or tostring(value) end)
  end
  if car.hasLowBeams then
    choice('beams', 'Headlight beam', car.lowBeams and 0 or 1, {'Low', 'High'},
      function(value) ac.setHighBeams(value == 1) end)
  end
  if car.hasTurningLights then
    local mode = car.hazardLights and ac.TurningLights.Hazards
      or car.turningLeftOnly and ac.TurningLights.Left
      or car.turningRightOnly and ac.TurningLights.Right or ac.TurningLights.None
    choice('signals', 'Indicators', mode, {'Off', 'Left', 'Right', 'Hazards'}, ac.setTurningLights)
  end
  if car.hasEngineBrakeSettings and car.engineBrakeSettingsCount > 0 then
    add('engine-brake', 'Engine braking', car.currentEngineBrakeSetting, 0,
      car.engineBrakeSettingsCount - 1, 1, ac.setEngineBrakeSetting)
  end
  if car.hasCockpitERSRecovery then
    add('ers-recovery', 'ERS recovery', car.mgukRecovery, 0, 10, 1, ac.setMGUKRecovery,
      function(value) return string.format('%d%%', value * 10) end)
  end
  if car.hasCockpitERSDelivery and car.mgukDeliveryCount > 0 then
    add('ers-delivery', 'ERS delivery', car.mgukDelivery, 0, car.mgukDeliveryCount - 1, 1, ac.setMGUKDelivery,
      function(value) return ac.getMGUKDeliveryName(0, value) or tostring(value) end)
  end
  if car.hasCockpitMGUHMode then
    choice('ers-heat', 'MGU-H energy', car.mguhChargingBatteries and 1 or 0, {'Motor', 'Battery'},
      function(value) ac.setMGUHCharging(value == 1) end)
  end
  -- Query actual pit options: unsupported tyres, repairs or wings are not invented.
  local pitOptions = ac.getPitstopSpinners()
  if #pitOptions > 0 then
    add('pit-preset', 'Pit strategy', sim.currentQuickPitPreset, 0, 2, 1, ac.setCurrentQuickPitPreset,
      function(value) return tostring(value + 1) end)
  end
  for _, spinner in ipairs(pitOptions) do
    local name = spinner.name
    local format
    if spinner.type == 'compound' then
      format = function(value)
        return value < 0 and 'No tyre change' or ac.getTyresLongName(0, value) or tostring(value)
      end
    elseif spinner.type == 'fuel' then
      format = function(value) return string.format('%d L', value) end
    elseif spinner.type == 'repair' and spinner.min == 0 and spinner.max == 1 then
      format = function(value) return value == 0 and 'No' or 'Yes' end
    end
    add('pit-' .. name, 'Pit · ' .. name, spinner.value, spinner.min, spinner.max, 1,
      function(value) return ac.setPitstopSpinnerValue(name, value) end, format)
    result[#result].readOnly = spinner.readOnly
  end
  add('fuel', 'Fuel remaining', car.fuel, 0, 0, 0, nil,
    function(value)
      local laps = car.fuelPerLap > 0 and string.format(' · %.1f laps', value / car.fuelPerLap) or ''
      return string.format('%.1f L%s', value, laps)
    end)
  command('ffb', 'Force feedback', percent(car.ffbMultiplier), '__EXT_FFB_DECREASE', '__EXT_FFB_INCREASE')
  local state = ac.getUI()
  choice('hud', 'Driving apps', state.appsHidden and 0 or 1, {'Hidden', 'Visible'},
    function(value) ac.setAppsHidden(value == 0) end)
  add('desktop', 'App page', state.currentDesktop, 0, 3, 1, ac.setCurrentDesktop,
    function(value) return tostring(value + 1) end)
  -- Keep cockpit editing out of the driving cycle. Selection is preserved by
  -- ID; if the car starts moving, these entries disappear and cannot fire.
  if math.abs(car.speedKmh) < 1 then
    local seat = ac.getOnboardCameraParams(0)
    command('seat-x', 'Seat · left / right', string.format('%.1f cm', seat.position.x * 100),
      '__EXT_SEAT_LEFT', '__EXT_SEAT_RIGHT')
    command('seat-y', 'Seat · down / up', string.format('%.1f cm', seat.position.y * 100),
      '__EXT_SEAT_DOWN', '__EXT_SEAT_UP')
    command('seat-z', 'Seat · back / forward', string.format('%.1f cm', seat.position.z * 100),
      '__EXT_SEAT_BACKWARD', '__EXT_SEAT_FORWARD')
    command('seat-pitch', 'Seat · pitch', string.format('%.1f°', seat.pitch),
      '__EXT_SEAT_PITCHDOWN', '__EXT_SEAT_PITCHUP')
    command('seat-yaw', 'Seat · yaw', string.format('%.1f°', seat.yaw),
      '__EXT_SEAT_YAWLEFT', '__EXT_SEAT_YAWRIGHT')
    command('seat-fov', 'Field of view', string.format('%.1f°', sim.firstPersonCameraFOV),
      '__EXT_SEAT_FOV_DECREASE', '__EXT_SEAT_FOV_INCREASE')
    action('seat-reset', 'Seat · reset', 'Factory position',
      function() ac.simulateCustomHotkeyPress('__EXT_SEAT_RESET', 1) end, 'Press right dial: reset seat')
    for index = 0, ac.getRealMirrorCount() - 1 do
      local mirrorIndex = index
      local params = ac.getRealMirrorParams(index)
      for _, axis in ipairs({'x', 'y'}) do
        local field = axis
        result[#result + 1] = {
          id = 'mirror-' .. index .. '-' .. field,
          label = 'Mirror ' .. (index + 1) .. (field == 'x' and ' · horizontal' or ' · vertical'),
          value = string.format('%.2f', params.rotation[field]), readOnly = params.notDrivenByUser,
          adjust = function(direction)
            local current = ac.getRealMirrorParams(mirrorIndex):clone()
            current.rotation[field] = current.rotation[field] + direction * 0.01
            ac.setRealMirrorParams(mirrorIndex, current, true)
          end,
        }
      end
    end
  end
  return result
end

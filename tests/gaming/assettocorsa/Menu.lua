local Menu = dofile(arg[1])
local menu = Menu.new()
local written = {}
local function setter(value) written[#written + 1] = value end
local tc = {id = 'tc', value = 2, min = 0, max = 3, step = 1, set = setter}
local fuel = {id = 'fuel', value = 10}
menu:replace({tc, fuel})
assert(menu:current() == tc)
assert(not menu:adjust(1, false) and #written == 0)
assert(menu:adjust(1, true) and written[1] == 3)
assert(tc.value == 2) -- wait for game acknowledgement
tc.value = 3
assert(not menu:adjust(1, true) and #written == 1)
tc.value = 0
assert(not menu:adjust(-1, true) and #written == 1)
menu:select(-1)
assert(menu:current() == fuel and not menu:adjust(1, true))
menu:select(1)
assert(menu:current() == tc)
menu:replace({fuel, tc})
assert(menu:current() == tc) -- preserve identity as car capabilities change
tc.readOnly = true
assert(not menu:adjust(1, true) and #written == 1)
menu:replace({fuel})
assert(menu:current() == fuel)
menu:replace({})
menu:select(1)
assert(menu:current() == nil and not menu:adjust(1, true))
local activated = 0
local command = {id = 'command', activate = function() activated = activated + 1 end}
menu:replace({command})
assert(not menu:activate(false) and activated == 0)
command.readOnly = true
assert(not menu:activate(true) and activated == 0)
command.readOnly = false
assert(menu:activate(true) and activated == 1)
assert(not menu:adjust(1, true))
print('PASS: menu selection, capability changes, bounds, read-only and game acknowledgement')

-- Input-independent selection model. CSP supplies live values and setters.
local Menu = {}
Menu.__index = Menu

function Menu.new()
  return setmetatable({entries = {}, selected = nil, visible = true}, Menu)
end

function Menu:replace(entries)
  self.entries = entries
  for _, entry in ipairs(entries) do
    if entry.id == self.selected then return end
  end
  self.selected = entries[1] and entries[1].id or nil
end

function Menu:current()
  for index, entry in ipairs(self.entries) do
    if entry.id == self.selected then return entry, index end
  end
end

function Menu:select(direction)
  if #self.entries == 0 then return end
  local _, index = self:current()
  index = ((index or 1) - 1 + direction) % #self.entries + 1
  self.selected = self.entries[index].id
  self.visible = true
end

function Menu:adjust(direction, allowed)
  local entry = self:current()
  if not allowed or not entry or entry.readOnly then return false end
  if entry.adjust then
    self.visible = true
    return entry.adjust(direction) ~= false
  end
  if not entry.set then return false end
  local value = math.max(entry.min, math.min(entry.max, entry.value + direction * entry.step))
  self.visible = true
  if value == entry.value then return false end
  -- Show the next value only when the game reports it on a subsequent frame.
  return entry.set(value) ~= false
end

function Menu:activate(allowed)
  local entry = self:current()
  if not allowed or not entry or entry.readOnly or not entry.activate then return false end
  self.visible = true
  return entry.activate() ~= false
end

return Menu

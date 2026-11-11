dofile(assert(arg[1]))
local function rect(x,y,w,h) return {x=x,y=y,w=w,h=h} end
local function overlap(a,b) return a.x < b.x+b.w and b.x < a.x+a.w and a.y < b.y+b.h and b.y < a.y+a.h end
local displays={Desk={6016,3384},TV={3840,2160},Handheld={1280,800}}
local count=0
for name,p in pairs(FlareUI2_DB.profiles) do
    local display=name:match('^(%w+) / ')
    if display then
        local physical=assert(displays[display]);local H=768/p.displayLayout.scale;local W=H*physical[1]/physical[2]
        local chat=p.displayLayout.chat;local uf=p.unitframes;local positions=uf.layouts.Desktop
        local r={chat=rect(chat.x,chat.y,chat.width,chat.height+28),actions=rect(W/2-280,0,560,p.actionbars.enabled and 174 or 220),minimap=rect(W-258,24,234,234)}
        if p.actionbars.enabled then
            local xp=p.actionbars.xpbar;local a=xp.layouts.Desktop
            r.xp=rect(W/2+a.x-xp.width/2,a.y,xp.width,xp.height)
            r.petactions=rect(chat.x,chat.y+chat.height+40,220,24)
        end
        for _,unit in ipairs({'player','target'}) do
            local cfg=uf.units[unit];local a=positions[unit]
            r[unit]=rect(W/2+a.x-cfg.width/2,a.y,cfg.width,cfg.height or 60)
        end
        r.targetauras=rect(r.target.x,r.target.y+r.target.h+4,r.target.w,88)
        r.targetcast=rect(r.target.x,r.target.y-21,r.target.w,18)
        for _,unit in ipairs({'pet','targettarget'}) do
            local cfg=uf.units[unit];local a=positions[unit];local parent=assert(r[a.relativeUnit])
            local width=cfg.width or (unit=='pet' and 120 or 100)
            local x=a.point=='TOPRIGHT' and parent.x+parent.w-width or parent.x
            r[unit]=rect(x+a.x,parent.y+a.y-cfg.height,width,cfg.height)
        end
        local cast=uf.playerCastbar;local a=positions.playercastbar
        r.cast=rect(W/2+a.x-(cast.width+32)/2,a.y,cast.width+32,cast.height+4)
        for _,key in ipairs({'combo','mana'}) do
            local cfg=uf.resource.bars[key];local a=uf.resource.layouts.Desktop[key]
            r[key]=rect(W/2+a.x-cfg.width/2,a.y,cfg.width,cfg.height or 12)
        end
        r.totems=rect(W/2-70,uf.totemLayouts.Desktop.y,140,30)
        r.focus=rect(W/2-(uf.units.focus.width or 160)/2,232,(uf.units.focus.width or 160),94)
        local q=p.objectivetracker;local qp=q.layouts.Desktop
        r.tracker=rect(W+qp.x-q.width,qp.y,q.width,q.maxHeight)
        assert(q.width==234 and qp.y==270 and (p.minimap.style or "square")=="square", "map/tracker column")
        for _,kind in ipairs({'buffs','debuffs'}) do
            local cfg=p.auras[kind];local a=p.auras.layouts.Desktop[kind]
            local spacing=cfg.spacing or 4
            local w=cfg.perRow*cfg.size+(cfg.perRow-1)*spacing
            local rows=math.ceil(cfg.max/cfg.perRow);local h=rows*cfg.size+(rows-1)*spacing
            if kind=='buffs' then
                assert(a.anchor=='playerHealth' and cfg.grow=='RIGHT' and cfg.wrap=='UP' and rows==2)
                r[kind]=rect(r.player.x+4,r.player.y+r.player.h+4,w,h)
            else
                assert(cfg.center, 'debuff group must remain centered')
                r[kind]=rect(W/2+a.x-w/2,H+a.y-h,w,h)
            end
        end
        for _,group in ipairs({'solo','party','raid'}) do
            local boxes={};for k,v in pairs(r) do boxes[k]=v end
            if group~='solo' then
                local cfg=uf[group];local tile=group=='party' and cfg.raid or cfg
                local w=group=='party' and (tile.width or 200) or tile.width*8+(tile.groupSpacing or 2)*7
                local h=group=='party' and (tile.height or 70)*5+(tile.spacing or 4)*4 or tile.height*5+(tile.spacing or 2)*4
                boxes[group]=rect(24,H-80-h,w,h)
                local b=uf.boss;local h=b.height*5+(b.spacing or 14)*4+b.castHeight+4
                boxes.boss=rect(W-310-b.width,H-24-h,b.width,h)
            end
            for key,a in pairs(boxes) do
                assert(a.x>=0 and a.y>=0 and a.x+a.w<=W and a.y+a.h<=H,name..': offscreen '..key)
                for other,b in pairs(boxes) do
                    if key<other then assert(not overlap(a,b),name..' '..group..': '..key..' overlaps '..other) end
                end
            end
            count=count+1
        end
        assert(uf.font.size==13 and uf.fontPower.size==12 and uf.levelFont.size==12 and uf.levelScale==.78, 'shared text hierarchy')
        assert(uf.units.player.width==180 and uf.units.player.height==54 and uf.units.player.ringSize==70, 'compact player/target proportions')
        assert(uf.units.target.ringSize==70 and q.wrapText~=false and q.titleFont.size==13 and q.headerFont.size==15)
        assert(math.abs(p.minimap.zoneFont.size*.9-13)<.001, 'map font compensates its local scale')
        assert(uf.styleStart and uf.tileStyle and uf.plateTiles, 'first-run style setup must not erase these positions')
        assert(not p.chat.saveHistory,'do not capture chat transcripts')
        assert((p.actionbars.enabled or false) == (p.displayLayout.input=='keyboard'))
    end
end
assert(count==18)
print('PASS: saved profile envelopes fit all three displays in solo, party and 40-player raid layouts')

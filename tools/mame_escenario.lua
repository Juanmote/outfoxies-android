-- Referencia con MAME (apt install mame): juega con el bot de headless, invulnerable, y lleva al escenario STAGE.
-- STAGE=7 SNAP0=700 SNAPN=60 ENDF=3000 mame outfxies -rompath roms -video none -sound none -nothrottle -skip_gameinfo \
--   -snapshot_directory snap -autoboot_script tools/mame_escenario.lua
local STAGE = tonumber(os.getenv("STAGE") or "7")
local SNAP0 = tonumber(os.getenv("SNAP0") or "700")
local SNAPN = tonumber(os.getenv("SNAPN") or "60")
local ENDF = tonumber(os.getenv("ENDF") or "3000")
local play = 300
local fr = 0
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local P = manager.machine.ioport.ports
local F = {
  right = P[":P1"].fields["P1 Right"], left = P[":P1"].fields["P1 Left"], down = P[":P1"].fields["P1 Down"],
  up = P[":P1"].fields["P1 Up"], b1 = P[":P1"].fields["P1 Button 1"], b2 = P[":P1"].fields["P1 Button 2"],
  b3 = P[":P1"].fields["P1 Button 3"], start = P[":P1"].fields["1 Player Start"], coin = P[":MISC"].fields["Coin 1"] }
local last7b = -1
local function setin(v)
  F.right:set_value(v & 1); F.left:set_value((v >> 1) & 1); F.down:set_value((v >> 2) & 1); F.up:set_value((v >> 3) & 1)
  F.b1:set_value((v >> 4) & 1); F.b2:set_value((v >> 5) & 1); F.b3:set_value((v >> 6) & 1)
end
sub = emu.add_machine_frame_notifier(function()
  fr = fr + 1
  F.coin:set_value(((fr >= play and fr < play + 4) or (fr >= play + 60 and fr < play + 64)) and 1 or 0)
  F.start:set_value((fr >= play + 120 and fr < play + 126) and 1 or 0)
  local p1 = 0
  if fr >= play + 200 and (fr % 30) < 15 then
    local seed = ((fr // 30) * 2654435761) & 0xffffffff
    p1 = (seed >> 7) & 0x7f
    if (fr // 30) % 3 == 0 then p1 = p1 | 0x10 end
  end
  setin(p1)
  local e = sp:read_u8(0x208d59)
  if e > 0 and e < 0xf0 then sp:write_u8(0x208d59, 0xf0) end
  local b7 = sp:read_u8(0x20807b)
  if b7 ~= last7b and sp:read_u8(0x20807d) ~= 0 then
    local cur, idx, pos = b7 >> 4, sp:read_u8(0x20808f), -1
    for k = 0, 6 do if sp:read_u8(0x20807d + k * 2) == STAGE and sp:read_u8(0x20807c + k * 2) == 0xff then pos = k end end
    if cur ~= STAGE and pos >= 0 and idx < 7 and sp:read_u8(0x20807d + idx * 2) == cur and sp:read_u8(0x20807c + idx * 2) == 0xff then
      sp:write_u8(0x20807d + pos * 2, cur); sp:write_u8(0x20807d + idx * 2, STAGE)
      sp:write_u8(0x20807b, (STAGE << 4) | (b7 & 15))
    end
  end
  last7b = sp:read_u8(0x20807b)
  if fr >= SNAP0 and (fr - SNAP0) % SNAPN == 0 then manager.machine.video:snapshot() end
  if fr >= ENDF then manager.machine:exit() end
end)

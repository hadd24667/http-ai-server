wrk.method = "GET"

local tick = 0

request = function()
  tick = tick + 1

  if tick % 4 == 0 then
    -- nhẹ
    return wrk.format(nil, "/xx")
  elseif tick % 4 == 1 then
    -- trung bình
    return wrk.format(nil, "/" .. string.rep("b", 25))
  elseif tick % 4 == 2 then
    -- nặng
    return wrk.format(nil, "/" .. string.rep("c", 80))
  else
    -- cực nặng
    return wrk.format(nil, "/" .. string.rep("d", 150))
  end
end

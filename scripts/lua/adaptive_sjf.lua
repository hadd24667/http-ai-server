wrk.method = "GET"

paths = {
  "/a",                                 -- nhẹ nhất
  "/bb",
  "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",    -- nặng
  "/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" -- rất nặng
}

math.randomseed(os.time())

request = function()
  local i = math.random(1, #paths)
  return wrk.format(nil, paths[i])
end

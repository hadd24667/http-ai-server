wrk.method = "GET"

-- giả lập weight bằng độ dài path
-- request dài = weight thấp, request ngắn = weight cao
paths = {
  "/x",         -- weight 1
  "/xxxxx",     -- weight 5
  "/xxxxxxxxxxxxx", -- weight 13
  "/xxxxxxxxxxxxxxxxxxxxxxxx", -- weight 24
}

math.randomseed(os.time())

request = function()
  local p = paths[math.random(1, #paths)]
  return wrk.format(nil, p)
end

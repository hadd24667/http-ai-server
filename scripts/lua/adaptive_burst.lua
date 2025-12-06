wrk.method = "GET"

function gen(n)
  return "/" .. string.rep("a", n)
end

paths = {}

-- burst nhẹ
for i = 1, 200 do
  table.insert(paths, gen(math.random(1, 5)))
end

-- burst cực lớn
for i = 1, 100 do
  table.insert(paths, gen(math.random(80, 200)))
end

-- trung bình
for i = 1, 100 do
  table.insert(paths, gen(math.random(20, 40)))
end

math.randomseed(os.time())

request = function()
  return wrk.format(nil, paths[math.random(#paths)])
end

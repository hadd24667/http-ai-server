wrk.method = "GET"

-- 4 nhóm tải:
-- Nhẹ (1–3 ký tự)  → FIFO ưu tiên
-- Trung bình (10–20 ký tự) → giúp queue moderate → SJF
-- Nặng (40–60 ký tự) → CPU tăng → RR
-- Siêu nặng (100–200 ký tự) → CPU rất cao → WFQ

function gen(size)
  return "/" .. string.rep("x", size)
end

paths = {}

-- Nhóm 1: rất nhẹ
for i = 1, 30 do
  table.insert(paths, gen(math.random(1, 3)))
end

-- Nhóm 2: trung bình
for i = 1, 30 do
  table.insert(paths, gen(math.random(10, 20)))
end

-- Nhóm 3: nặng
for i = 1, 30 do
  table.insert(paths, gen(math.random(40, 60)))
end

-- Nhóm 4: cực nặng
for i = 1, 30 do
  table.insert(paths, gen(math.random(100, 200)))
end

math.randomseed(os.time())

request = function()
  local p = paths[math.random(1, #paths)]
  return wrk.format(nil, p)
end

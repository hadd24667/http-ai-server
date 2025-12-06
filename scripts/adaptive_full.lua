wrk.method = "GET"

-- Path pool để tạo workload khác nhau 
local paths = {
    "/x", "/xxx", "/abc/abcdef",
    "/big/request/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "/tiny",
    "/super/large/request/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
}

math.randomseed(os.time())

request = function()
    local idx = math.random(1, #paths)
    return wrk.format(nil, paths[idx])
end

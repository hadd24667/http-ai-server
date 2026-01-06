-- wrk_test.lua
math.randomseed(os.time())

local paths = {
    "/",
    "/health",
    "/api/data",
    "/api/compute",
    "/api/heavy/work"
}

request = function()
    local path = paths[math.random(#paths)]

    local method = (math.random() < 0.75) and "GET" or "POST"

    local headers = {
        ["Host"] = "127.0.0.1",
        ["Content-Type"] = "application/json"
    }

    local body = nil
    if method == "POST" then
        -- giả lập request nặng/nhẹ khác nhau
        body = string.rep("x", math.random(50, 3000))
    end

    return wrk.format(method, path, headers, body)
end

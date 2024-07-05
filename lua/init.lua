--require("plugins.blue_strobe")
--require("plugins.triangle")
--require("plugins.waveform")

O = lynx.opt
A = lynx.api
R = lynx.api.renderer
P = lynx.api.param

O.bg_color = {1, 1, 25, 255}

local yoompa = P.add("yoompa", 10, 0, 100)
local frequencies = {}
local screen_size = {}

lynx.api.on_render(function()
end);

lynx.api.on_update(function() 
    frequencies = A.get_sample_frequencies()
    screen_size = A.get_screen_size()
end);

--require("plugins.blue_strobe")
--require("plugins.triangle")
--require("plugins.waveform")

O = lynx.opt
A = lynx.api
R = lynx.api.renderer
P = lynx.api.param
Pr = lynx.api.proc

O.bg_color = {1, 1, 25, 255}

local custom_parameter = P.add("CustomParameter", 10, 0, 100)
local frequencies = {}
local screen_size = {}

local custom_procedure = Pr.add("CustomProcedure", function()
    R.draw_centered_text("Position controlled by CustomParameter", 
    			 {400, 400+P.get(custom_parameter)}, 25)
end)

lynx.api.on_render(function()
    Pr.call(custom_procedure)
end);

lynx.api.on_update(function() 
    frequencies = A.get_sample_frequencies()
    screen_size = A.get_screen_size()
end);


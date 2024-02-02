A = lynx.api

local vertices = {}
local indices = {}
local screen_size

A.animation.add("test", function(animation, _)
    local elapsed = A.animation.get_elapsed(animation)

    A.animation.set_val(animation, math.sin(elapsed)*0.5 + 0.5)

    if elapsed > 3.1415*2 then
        A.animation.set_finished(animation)
    end
end)

A.add_param("font_size", 20, 10, 100)

A.on_update(function()
    screen_size = A.get_screen_size()

    local samples = A.get_samples()

    for i = 1, screen_size[1] do
        local y = i - 1

        vertices[i] = {y*A.get_param("wave_width"), samples[#samples - i + 1]*screen_size[2]/2 + screen_size[2]/2}
        indices[i] = {y, samples[#samples - i + 1]}
    end
end)


A.pre_render(function()
    A.bind_shader(0, "assets/shaders/red.fs")

    A.draw_lined_poly(vertices, indices, {255, 255, 255, 255})

    A.unbind_shader()

    A.draw_centered_text("Waveform", {255, 255*A.animation.load("test", 0.5)}, A.get_param("font_size"))
end)

function table.copy(t)
    local u = { }
    for k, v in pairs(t) do u[k] = v end
    return setmetatable(u, getmetatable(t))
end

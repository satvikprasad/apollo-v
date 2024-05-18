-- Have to debug this

A = lynx.api

S = {
    vertices = {},
    indices = {},
    screen_size = {},
}

local sin_animation = A.animation.add("Sin Text", function(animation, _)
    local elapsed = A.animation.get_elapsed(animation)

    A.animation.set_val(animation, math.sin(elapsed/2)*0.5 + 0.5)
end)

local font_size = A.param.add("Font Size", 20, 10, 100)
local wave_width = A.param.add("Wave Width", 10, 1, 100)

local portishead_proc = A.proc.add("Portishead?", function()
    local X = (S.screen_size.x) * A.animation.load(sin_animation, 0.5)

    local height = 30

    for i = 1, S.screen_size.y/height do
        A.renderer.draw_centered_text("Portis-Head?", {X, i*height}, height)
        A.renderer.draw_centered_text("Portis-Head?", {S.screen_size.x - X, height/2 + i*height}, height)
    end
end)

local red_waveform_proc = A.proc.add("_3", function()
    A.bind_shader(0, "red.fs")

    A.renderer.draw_lined_poly(S.vertices, S.indices, {255, 255, 255, 255})

    A.unbind_shader()
end)

A.on_update(function()
    S.screen_size = A.get_screen_size()

    local samples = A.get_samples()

    for i = 1, S.screen_size.x do
        local y = i - 1

        S.vertices[i] = {y*A.param.get(wave_width), samples[#samples - i + 1]*S.screen_size.y/2 + S.screen_size.y/2}
        S.indices[i] = {y, samples[#samples - i + 1]}
    end
end)

A.pre_render(function()
    --A.proc.call(red_waveform_proc)

    A.proc.call(portishead_proc)
end)

function table.copy(t)
    local u = { }
    for k, v in pairs(t) do u[k] = v end
    return setmetatable(u, getmetatable(t))
end

A = lynx.api

S = {
    vertices = {},
    indices = {},
    screen_size = {},
}

local font_size = A.param.add("FontSize", 20, 10, 100)
local wave_width = A.param.add("WaveWidth", 10, 1, 100)

local red_waveform_proc = A.proc.add("RedWaveform", function()
    A.bind_shader(0, "red.fs")

    A.renderer.draw_lined_poly(S.vertices, S.indices, {255, 255, 255, 255})

    A.unbind_shader()
end)

local sin_animation = A.animation.add("SinText", function(animation, _)
    local elapsed = A.animation.get_elapsed(animation)

    A.animation.set_val(animation, math.sin(elapsed)*0.5 + 0.5)
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
    A.proc.call(red_waveform_proc)

    A.renderer.draw_centered_text("Vizzy.", {255, 100 + 255*A.animation.load(sin_animation, 0.5)}, A.param.get(font_size))
end)

function table.copy(t)
    local u = { }
    for k, v in pairs(t) do u[k] = v end
    return setmetatable(u, getmetatable(t))
end

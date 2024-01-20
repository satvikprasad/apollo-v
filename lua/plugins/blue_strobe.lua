A = lynx.api

A.add_param("bpm", 76, 0, 200)

A.on_update(function()
    local time_played = A.get_music_time_played()
    local bpm = A.get_param("bpm")

    local bg_color = A.get_bg_color()

    local blue = 50*math.sin(1*(bpm/60)*time_played*2*math.pi) + 50

    A.set_bg_color({bg_color.r, bg_color.g, blue, bg_color.a})
end)

print("hello from blue_strobe.lua\n\n\n");

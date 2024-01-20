A = lynx.api

A.add_param("triangle_y", 0, 0, 720);

A.on_render(function()
    local triangle_y = A.get_param("triangle_y")
    local white = {255, 255, 255, 255}

    A.draw_lined_poly({{100, 100 + triangle_y}, {100, 150 + triangle_y}, {150, 100 + triangle_y}, {100, 100 + triangle_y}}, {{0, 0}, {0, 1}, {1, 0}, {0, 0}}, white)
end)

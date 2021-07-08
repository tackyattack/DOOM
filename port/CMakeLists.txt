add_library(doom_port_lib STATIC
i_net.c
i_sound.c
i_system.c
i_video.c
)

target_link_libraries(doom_port_lib doom_lib)
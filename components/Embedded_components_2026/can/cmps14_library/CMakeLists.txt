idf_component_register(SRCS "cmps14_library.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       REQUIRES driver i2c_library)

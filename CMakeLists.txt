# DHT11 Temperature and Humidity Sensor Driver Component
message(STATUS "ADDING COMPONENT: DHT11 Sensor Driver")

# Create component for DHT11 driver
idf_component_register(

    SRCS
        "src/dht11.c"

    INCLUDE_DIRS
        "include"

    REQUIRES
        my_basic_hal
        
    PRIV_REQUIRES
        esp_timer

)
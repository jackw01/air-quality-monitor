#include <homekit/homekit.h>
#include <homekit/characteristics.h>

void my_accessory_identify(homekit_value_t _value) {
  // called when accessory is paired
}

homekit_characteristic_t cha_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t cha_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t cha_co2_detected = HOMEKIT_CHARACTERISTIC_(CARBON_DIOXIDE_DETECTED, 0);
homekit_characteristic_t cha_co2_level = HOMEKIT_CHARACTERISTIC_(CARBON_DIOXIDE_LEVEL, 0);
homekit_characteristic_t cha_air_quality = HOMEKIT_CHARACTERISTIC_(AIR_QUALITY, 0);
homekit_characteristic_t cha_pm25 = HOMEKIT_CHARACTERISTIC_(PM25_DENSITY, 0);
homekit_characteristic_t cha_voc = HOMEKIT_CHARACTERISTIC_(VOC_DENSITY, 0);

// (optional) format: bool; HAP section 9.96
// homekit_characteristic_t cha_status_active = HOMEKIT_CHARACTERISTIC_(STATUS_ACTIVE, true);

// (optional) format: uint8; HAP section 9.97; 0 "No Fault", 1 "General Fault"
// homekit_characteristic_t cha_status_fault = HOMEKIT_CHARACTERISTIC_(STATUS_FAULT, 0);

// (optional) format: uint8; HAP section 9.100; 0 "Accessory is not tampered", 1 "Accessory is tampered with"
// homekit_characteristic_t cha_status_tampered = HOMEKIT_CHARACTERISTIC_(STATUS_TAMPERED, 0);


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Air Quality Monitor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "jackw01"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1"),
            HOMEKIT_CHARACTERISTIC(MODEL, "1.0"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
            &cha_temperature,
            //&cha_status_active,//optional
            //&cha_status_fault,//optional
            //&cha_status_tampered,//optional
            NULL
        }),
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
            &cha_humidity,
            NULL
        }),
        HOMEKIT_SERVICE(CARBON_DIOXIDE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "CO2 Sensor"),
            &cha_co2_detected,
            &cha_co2_level,
            NULL
        }),
        HOMEKIT_SERVICE(AIR_QUALITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Air Quality"),
            &cha_air_quality,
            &cha_pm25,
            &cha_voc,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

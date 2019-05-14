#include <node_api.h>
#include <napi-macros.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>

#define NAPI_RETURN_THROWS(call, message) \
  if ((call)) { \
    napi_throw_error(env, NULL, message); \
    return NULL; \
  }

NAPI_METHOD(enumerate) {
  NAPI_ARGV(2)
  // TODO: Needs type protection, eg. null/undefined
  NAPI_ARGV_INT32(vendor_id, 0)
  NAPI_ARGV_INT32(product_id, 1)

  char wide_buffer[1024 + 1];
  struct hid_device_info * device = hid_enumerate(vendor_id, product_id);

  napi_value devices;
  NAPI_STATUS_THROWS(napi_create_array(env, &devices))

  if (device == NULL) {
    return devices;
  }

  uint32_t i = 0;
  do {
    napi_value device_obj;
    NAPI_STATUS_THROWS(napi_create_object(env, &device_obj))

    napi_value path_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "path", NAPI_AUTO_LENGTH, &path_key))
    napi_value path_value;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, device->path, NAPI_AUTO_LENGTH, &path_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, path_key, path_value))

    napi_value vendor_id_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "vendor_id", NAPI_AUTO_LENGTH, &vendor_id_key))
    napi_value vendor_id_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->vendor_id, &vendor_id_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, vendor_id_key, vendor_id_value))

    napi_value product_id_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "product_id", NAPI_AUTO_LENGTH, &product_id_key))
    napi_value product_id_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->product_id, &product_id_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, product_id_key, product_id_value))

    napi_value serial_number_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "serial_number", NAPI_AUTO_LENGTH, &serial_number_key))
    napi_value serial_number_value;

    if (wcstombs(wide_buffer, device->serial_number, 1024) < 0) {
      napi_throw_error(env, NULL, "");
      hid_free_enumeration(device);
      return NULL;
    }

    NAPI_STATUS_THROWS(napi_create_string_utf8(env, wide_buffer, NAPI_AUTO_LENGTH, &serial_number_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, serial_number_key, serial_number_value))

    napi_value release_number_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "release_number", NAPI_AUTO_LENGTH, &release_number_key))
    napi_value release_number_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->release_number, &release_number_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, release_number_key, release_number_value))

    napi_value manufacturer_string_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "manufacturer_string", NAPI_AUTO_LENGTH, &manufacturer_string_key))
    napi_value manufacturer_string_value;

    if (wcstombs(wide_buffer, device->manufacturer_string, 1024) < 0) {
      napi_throw_error(env, NULL, "");
      hid_free_enumeration(device);
      return NULL;
    }

    NAPI_STATUS_THROWS(napi_create_string_utf8(env, wide_buffer, NAPI_AUTO_LENGTH, &manufacturer_string_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, manufacturer_string_key, manufacturer_string_value))

    napi_value product_string_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "product_string", NAPI_AUTO_LENGTH, &product_string_key))
    napi_value product_string_value;

    if (wcstombs(wide_buffer, device->product_string, 1024) < 0) {
      napi_throw_error(env, NULL, "");
      hid_free_enumeration(device);
      return NULL;
    }

    NAPI_STATUS_THROWS(napi_create_string_utf8(env, wide_buffer, NAPI_AUTO_LENGTH, &product_string_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, product_string_key, product_string_value))

    napi_value usage_page_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "usage_page", NAPI_AUTO_LENGTH, &usage_page_key))
    napi_value usage_page_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->usage_page, &usage_page_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, usage_page_key, usage_page_value))

    napi_value usage_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "usage", NAPI_AUTO_LENGTH, &usage_key))
    napi_value usage_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->usage, &usage_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, usage_key, usage_value))

    napi_value interface_number_key;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "interface_number", NAPI_AUTO_LENGTH, &interface_number_key))
    napi_value interface_number_value;
    NAPI_STATUS_THROWS(napi_create_int32(env, device->interface_number, &interface_number_value))
    NAPI_STATUS_THROWS(napi_set_property(env, device_obj, interface_number_key, interface_number_value))

    NAPI_STATUS_THROWS(napi_set_element(env, devices, i++, device_obj))
  } while ((device = device->next) != NULL);

  hid_free_enumeration(device);

  return devices;
}

NAPI_INIT() {
  if (hid_init() != 0) {
    napi_throw_error(env, NULL, "Failed to hid_init");
    return;
  }

  NAPI_EXPORT_FUNCTION(enumerate);

  NAPI_STATUS_THROWS(napi_add_env_cleanup_hook(env, (void *)hid_exit, NULL));
}

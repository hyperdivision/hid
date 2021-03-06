#include <node_api.h>
#include <napi-macros.h>
#ifdef _WIN32
  #include <windows.h>
#endif
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>

#define NAPI_RETURN_THROWS(call, message) \
  if ((call)) { \
    napi_throw_error(env, NULL, message); \
    return NULL; \
  }

#define NAPI_EXTERNAL(name, val) \
  NAPI_EXTERNAL_CAST(void *, name, val)

#define NAPI_TYPEOF(name, val) \
  napi_valuetype name##_valuetype; \
  NAPI_STATUS_THROWS(napi_typeof(env, val, &name##_valuetype));

#define NAPI_ASSERT_TYPED_ARRAY(name, val, message) \
  bool name##_is_typedarray; \
  NAPI_STATUS_THROWS(napi_is_typedarray(env, val, &name##_is_typedarray)) \
  if (name##_is_typedarray == 0) { \
    napi_throw_type_error(env, NULL, message); \
    return NULL; \
  }

#define NAPI_ASSERT_ARGV_TYPED_ARRAY(name, i, message) \
  NAPI_ASSERT_TYPED_ARRAY(name, argv[i], message)

#define NAPI_INT32_OPT(name, val, default) \
  NAPI_TYPEOF(name, val) \
  int32_t name; \
  if (name##_valuetype == napi_null || name##_valuetype == napi_undefined) { \
    name = default; \
  } else if (napi_get_value_int32(env, val, &name) != napi_ok) { \
    napi_throw_error(env, "EINVAL", "Expected number"); \
    return NULL; \
  }

#define NAPI_ARGV_INT32_OPT(name, i, default) \
  NAPI_INT32_OPT(name, argv[i], default)

#define NAPI_ARGV_UTF8_OPT(name, size, i, default, default_size) \
  NAPI_UTF8_OPT(name, size, argv[i], default, default_size)

#define NAPI_EXTERNAL_CAST(type, name, val) \
  type name; \
  if (napi_get_value_external(env, val, (void **) &name) != napi_ok) { \
    napi_throw_error(env, "EINVAL", "Expected external"); \
    return NULL; \
  }

#define NAPI_RETURN_EXTERNAL(name, finalizer, finalizer_hint) \
  napi_value return_external; \
  NAPI_STATUS_THROWS(napi_create_external(env, name, finalizer, finalizer_hint, &return_external)) \
  return return_external;

#define NAPI_ARGV_EXTERNAL(name, i) \
  NAPI_EXTERNAL(name, argv[i])

#define NAPI_ARGV_EXTERNAL_CAST(type, name, i) \
  NAPI_EXTERNAL_CAST(type, name, argv[i])

#define THROW_HID_ERROR(handle, condition, default) \
  if ((condition)) { \
    char err_mbs_buffer[1024 + 1]; \
    const wchar_t* err = hid_error(handle); \
    NAPI_RETURN_THROWS(err == NULL, default); \
    NAPI_RETURN_THROWS(wcstombs(err_mbs_buffer, err, 1024) == SIZE_MAX, "Failed convert error"); \
    napi_throw_error(env, NULL, err_mbs_buffer); \
    return NULL; \
  }

#define NAPI_ASSERT_ARGV_MIN(n) \
  NAPI_RETURN_THROWS(argc < n, "Unsufficient arguments provided. Expected " #n)

#define NAPI_EXPORT_NAMED_FUNCTION(key, name) \
  { \
    napi_value name##_fn; \
    NAPI_STATUS_THROWS(napi_create_function(env, key, NAPI_AUTO_LENGTH, name, NULL, &name##_fn)) \
    NAPI_STATUS_THROWS(napi_set_named_property(env, exports, key, name##_fn)) \
  }

NAPI_METHOD(napi_hid_enumerate) {
  NAPI_ARGV(2)
  NAPI_ARGV_INT32_OPT(vendor_id, 0, 0)
  NAPI_ARGV_INT32_OPT(product_id, 1, 0)

  char mbs_buffer[1024 + 1];
  struct hid_device_info * device = hid_enumerate(vendor_id, product_id);
  struct hid_device_info * list = device;
  napi_value devices;
  NAPI_STATUS_THROWS(napi_create_array(env, &devices))

  if (device == NULL) {
    return devices;
  }

  uint32_t i = 0;
  do {
    napi_value device_obj;
    NAPI_STATUS_THROWS(napi_create_object(env, &device_obj))

    napi_value path_value;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, device->path, NAPI_AUTO_LENGTH, &path_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "path", path_value))

    napi_value vendor_id_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->vendor_id, &vendor_id_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "vendor_id", vendor_id_value))

    napi_value product_id_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->product_id, &product_id_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "product_id", product_id_value))


    if (wcstombs(mbs_buffer, device->serial_number, 1024) == SIZE_MAX) {
      napi_throw_error(env, NULL, "serial_number");
      hid_free_enumeration(list);
      return NULL;
    }

    napi_value serial_number_value;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, mbs_buffer, NAPI_AUTO_LENGTH, &serial_number_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "serial_number", serial_number_value))

    napi_value release_number_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->release_number, &release_number_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "release_number", release_number_value))


    if (wcstombs(mbs_buffer, device->manufacturer_string, 1024) == SIZE_MAX) {
      napi_throw_error(env, NULL, "manufacturer_string");
      hid_free_enumeration(list);
      return NULL;
    }

    napi_value manufacturer_string_value;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, mbs_buffer, NAPI_AUTO_LENGTH, &manufacturer_string_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "manufacturer_string", manufacturer_string_value))


    if (wcstombs(mbs_buffer, device->product_string, 1024) == SIZE_MAX) {
      napi_throw_error(env, NULL, "product_string");
      hid_free_enumeration(list);
      return NULL;
    }

    napi_value product_string_value;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, mbs_buffer, NAPI_AUTO_LENGTH, &product_string_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "product_string", product_string_value))

    napi_value usage_page_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->usage_page, &usage_page_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "usage_page", usage_page_value))

    napi_value usage_value;
    NAPI_STATUS_THROWS(napi_create_uint32(env, device->usage, &usage_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "usage", usage_value))

    napi_value interface_number_value;
    NAPI_STATUS_THROWS(napi_create_int32(env, device->interface_number, &interface_number_value))
    NAPI_STATUS_THROWS(napi_set_named_property(env, device_obj, "interface_number", interface_number_value))

    NAPI_STATUS_THROWS(napi_set_element(env, devices, i++, device_obj))
  } while ((device = device->next) != NULL);

  hid_free_enumeration(list);

  return devices;
}

void napi_hid_device_finalizer (napi_env env, void* finalize_data, void* finalize_hint) {
  hid_close((hid_device *) finalize_data);
}

NAPI_METHOD(napi_hid_open) {
  NAPI_ARGV(3)
  NAPI_ASSERT_ARGV_MIN(2)
  NAPI_ARGV_INT32(vendor_id, 0)
  NAPI_ARGV_INT32(product_id, 1)

  wchar_t * wserial_number = NULL;
  if (argc == 3) {
    wchar_t wide_buffer[sizeof(wchar_t) * 257];
    NAPI_ARGV_UTF8(serial_number, 1024 + 1, 2)
    NAPI_RETURN_THROWS(mbstowcs(wide_buffer, serial_number, sizeof(wchar_t) * 256) == SIZE_MAX, "Failed to convert serial number")
  }

  hid_device * device = hid_open(vendor_id, product_id, wserial_number);

  NAPI_RETURN_THROWS(device == NULL, "Failed open device")

  NAPI_RETURN_EXTERNAL(device, napi_hid_device_finalizer, NULL)
}

NAPI_METHOD(napi_hid_open_path) {
  NAPI_ARGV(1)
  NAPI_ASSERT_ARGV_MIN(1)
  NAPI_ARGV_UTF8(path, 1024 + 1, 0)

  hid_device * device = hid_open_path(path);

  NAPI_RETURN_THROWS(device == NULL, "Failed open_path device")

  NAPI_RETURN_EXTERNAL(device, napi_hid_device_finalizer, NULL)
}

NAPI_METHOD(napi_hid_write) {
  NAPI_ARGV(2)
  NAPI_ASSERT_ARGV_MIN(2)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  NAPI_ARGV_BUFFER_CAST(const unsigned char *, data, 1)

  int n = hid_write(handle, data, data_len);

  THROW_HID_ERROR(handle, n < 0, "Failed write")

  NAPI_RETURN_INT32(n)
}

NAPI_METHOD(napi_hid_read_timeout) {
  NAPI_ARGV(3)
  NAPI_ASSERT_ARGV_MIN(3)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  NAPI_ARGV_BUFFER_CAST(unsigned char *, data, 1)
  NAPI_ARGV_INT32(milliseconds, 2)

  int n = hid_read_timeout(handle, data, data_len, milliseconds);

  THROW_HID_ERROR(handle, n < 0, "Failed read_timeout")

  NAPI_RETURN_INT32(n)
}

NAPI_METHOD(napi_hid_read) {
  NAPI_ARGV(2)
  NAPI_ASSERT_ARGV_MIN(2)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  NAPI_ARGV_BUFFER_CAST(unsigned char *, data, 1)

  int n = hid_read(handle, data, data_len);

  THROW_HID_ERROR(handle, n < 0, "Failed read")

  NAPI_RETURN_INT32(n)
}

typedef struct async_read_request {
  hid_device * handle;
  napi_ref data_ref;
  unsigned char * data;
  size_t data_len;
  napi_ref cb;
  int n;
  napi_async_work task;
} async_read_request;

void async_read_execute(napi_env env, void* req_v) {
  struct async_read_request * req = (async_read_request *)req_v;
  req->n = hid_read(req->handle, req->data, req->data_len);
}

void async_read_complete(napi_env env, napi_status status, void* data) {
  async_read_request * req = (async_read_request *)data;
  NAPI_STATUS_THROWS(status);

  napi_value global;
  NAPI_STATUS_THROWS(napi_get_global(env, &global));

  napi_value argv[2];
  if (req->n < 0) {
    // TODO try to read error from hid
    napi_value err_msg;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "Failed read", NAPI_AUTO_LENGTH, &err_msg));
    NAPI_STATUS_THROWS(napi_create_error(env, NULL, err_msg, &argv[0]));
    NAPI_STATUS_THROWS(napi_get_undefined(env, &argv[1]));
  } else {
    NAPI_STATUS_THROWS(napi_get_null(env, &argv[0]));
    NAPI_STATUS_THROWS(napi_create_uint32(env, req->n, &argv[1]));
  }

  napi_value callback;
  NAPI_STATUS_THROWS(napi_get_reference_value(env, req->cb, &callback));

  napi_value return_val;
  NAPI_STATUS_THROWS(napi_call_function(env, global, callback, 2, argv, &return_val));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->cb));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->data_ref));
  NAPI_STATUS_THROWS(napi_delete_async_work(env, req->task));
  free(req);
}

NAPI_METHOD(napi_hid_read_async) {
  NAPI_ARGV(3)
  NAPI_ASSERT_ARGV_MIN(3)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  napi_value data_val = argv[1];
  napi_value cb = argv[2];

  async_read_request * req = (async_read_request *) malloc(sizeof(async_read_request));
  req->handle = handle;
  req->n = 0;

  NAPI_STATUS_THROWS(napi_create_reference(env, cb, 1, &req->cb));
  NAPI_STATUS_THROWS(napi_create_reference(env, data_val, 1, &req->data_ref));
  NAPI_BUFFER_CAST(unsigned char *, data, data_val)
  req->data = data;
  req->data_len = data_len;

  napi_value async_resource_name;
  NAPI_STATUS_THROWS(napi_create_string_utf8(env, "hid:read_async", NAPI_AUTO_LENGTH, &async_resource_name))

  napi_create_async_work(env, NULL, async_resource_name,
                                   async_read_execute,
                                   async_read_complete,
                                   (void*)req, &req->task);

  NAPI_STATUS_THROWS(napi_queue_async_work(env, req->task))

  return NULL;
}

typedef struct async_read_timeout_request {
  hid_device * handle;
  napi_ref data_ref;
  unsigned char * data;
  size_t data_len;
  int milliseconds;
  napi_ref cb;
  int n;
  napi_async_work task;
} async_read_timeout_request;

void async_read_timeout_execute(napi_env env, void* req_v) {
  struct async_read_timeout_request * req = (async_read_timeout_request *)req_v;
  req->n = hid_read_timeout(req->handle, req->data, req->data_len, req->milliseconds);
}

void async_read_timeout_complete(napi_env env, napi_status status, void* data) {
  async_read_timeout_request * req = (async_read_timeout_request *)data;
  NAPI_STATUS_THROWS(status);

  napi_value global;
  NAPI_STATUS_THROWS(napi_get_global(env, &global));

  napi_value argv[2];
  if (req->n < 0) {
    // TODO try to read error from hid
    napi_value err_msg;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "Failed read_timeout", NAPI_AUTO_LENGTH, &err_msg));
    NAPI_STATUS_THROWS(napi_create_error(env, NULL, err_msg, &argv[0]));
    NAPI_STATUS_THROWS(napi_get_undefined(env, &argv[1]));
  } else {
    NAPI_STATUS_THROWS(napi_get_null(env, &argv[0]));
    NAPI_STATUS_THROWS(napi_create_uint32(env, req->n, &argv[1]));
  }

  napi_value callback;
  NAPI_STATUS_THROWS(napi_get_reference_value(env, req->cb, &callback));

  napi_value return_val;
  NAPI_STATUS_THROWS(napi_call_function(env, global, callback, 2, argv, &return_val));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->cb));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->data_ref));
  NAPI_STATUS_THROWS(napi_delete_async_work(env, req->task));
  free(req);
}

NAPI_METHOD(napi_hid_read_timeout_async) {
  NAPI_ARGV(4)
  NAPI_ASSERT_ARGV_MIN(4)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  napi_value data_val = argv[1];
  NAPI_ARGV_INT32(milliseconds, 2)
  napi_value cb = argv[3];

  async_read_timeout_request * req = (async_read_timeout_request *) malloc(sizeof(async_read_timeout_request));
  req->handle = handle;
  req->milliseconds = milliseconds;
  req->n = 0;

  NAPI_STATUS_THROWS(napi_create_reference(env, cb, 1, &req->cb));
  NAPI_STATUS_THROWS(napi_create_reference(env, data_val, 1, &req->data_ref));
  NAPI_BUFFER_CAST(unsigned char *, data, data_val)
  req->data = data;
  req->data_len = data_len;

  napi_value async_resource_name;
  NAPI_STATUS_THROWS(napi_create_string_utf8(env, "hid:read_timeout_async", NAPI_AUTO_LENGTH, &async_resource_name))
  napi_create_async_work(env, NULL, async_resource_name,
                                   async_read_timeout_execute,
                                   async_read_timeout_complete,
                                   (void*)req, &req->task);

  NAPI_STATUS_THROWS(napi_queue_async_work(env, req->task))

  return NULL;
}

NAPI_METHOD(napi_hid_set_nonblocking) {
  NAPI_ARGV(2)
  NAPI_ASSERT_ARGV_MIN(2)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ARGV_INT32(nonblock, 1)

  int res = hid_set_nonblocking(handle, nonblock);

  THROW_HID_ERROR(handle, res < 0, "Failed hid_set_nonblocking")

  return NULL;
}

NAPI_METHOD(napi_hid_send_feature_report) {
  NAPI_ARGV(2)
  NAPI_ASSERT_ARGV_MIN(2)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  NAPI_ARGV_BUFFER_CAST(const unsigned char *, data, 1)

  int n = hid_send_feature_report(handle, data, data_len);

  THROW_HID_ERROR(handle, n < 0, "Failed send_feature_report")

  NAPI_RETURN_INT32(n)
}

NAPI_METHOD(napi_hid_get_feature_report) {
  NAPI_ARGV(2)
  NAPI_ASSERT_ARGV_MIN(2)
  // TODO: Needs type protection, eg. null/undefined
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  NAPI_ARGV_BUFFER_CAST(unsigned char *, data, 1)

  int n = hid_get_feature_report(handle, data, data_len);

  THROW_HID_ERROR(handle, n < 0, "Failed get_feature_report")

  NAPI_RETURN_INT32(n)
}

typedef struct async_get_feature_report_request {
  hid_device * handle;
  napi_ref data_ref;
  unsigned char * data;
  size_t data_len;
  int milliseconds;
  napi_ref cb;
  int n;
  napi_async_work task;
} async_get_feature_report_request;

void async_get_feature_report_execute(napi_env env, void* req_v) {
  struct async_get_feature_report_request * req = (async_get_feature_report_request *)req_v;
  req->n = hid_get_feature_report(req->handle, req->data, req->data_len);
}

void async_get_feature_report_complete(napi_env env, napi_status status, void* data) {
  async_get_feature_report_request * req = (async_get_feature_report_request *)data;
  NAPI_STATUS_THROWS(status);

  napi_value global;
  NAPI_STATUS_THROWS(napi_get_global(env, &global));

  napi_value argv[2];
  if (req->n < 0) {
    // TODO try to read error from hid
    napi_value err_msg;
    NAPI_STATUS_THROWS(napi_create_string_utf8(env, "Failed get_feature_report", NAPI_AUTO_LENGTH, &err_msg));
    NAPI_STATUS_THROWS(napi_create_error(env, NULL, err_msg, &argv[0]));
    NAPI_STATUS_THROWS(napi_get_undefined(env, &argv[1]));
  } else {
    NAPI_STATUS_THROWS(napi_get_null(env, &argv[0]));
    NAPI_STATUS_THROWS(napi_create_uint32(env, req->n, &argv[1]));
  }

  napi_value callback;
  NAPI_STATUS_THROWS(napi_get_reference_value(env, req->cb, &callback));

  napi_value return_val;
  NAPI_STATUS_THROWS(napi_call_function(env, global, callback, 2, argv, &return_val));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->cb));
  NAPI_STATUS_THROWS(napi_delete_reference(env, req->data_ref));
  NAPI_STATUS_THROWS(napi_delete_async_work(env, req->task));
  free(req);
}

NAPI_METHOD(napi_hid_get_feature_report_async) {
  NAPI_ARGV(3)
  NAPI_ASSERT_ARGV_MIN(3)
  NAPI_ARGV_EXTERNAL_CAST(hid_device *, handle, 0)
  NAPI_ASSERT_ARGV_TYPED_ARRAY(data, 1, "data must be Buffer")
  napi_value data_val = argv[1];
  napi_value cb = argv[2];

  async_get_feature_report_request * req = (async_get_feature_report_request *) malloc(sizeof(async_get_feature_report_request));
  req->handle = handle;
  req->n = 0;

  NAPI_STATUS_THROWS(napi_create_reference(env, cb, 1, &req->cb));
  NAPI_STATUS_THROWS(napi_create_reference(env, data_val, 1, &req->data_ref));
  NAPI_BUFFER_CAST(unsigned char *, data, data_val)
  req->data = data;
  req->data_len = data_len;

  napi_value async_resource_name;
  NAPI_STATUS_THROWS(napi_create_string_utf8(env, "hid:get_feature_report_async", NAPI_AUTO_LENGTH, &async_resource_name))
  napi_create_async_work(env, NULL, async_resource_name,
                                   async_get_feature_report_execute,
                                   async_get_feature_report_complete,
                                   (void*)req, &req->task);

  NAPI_STATUS_THROWS(napi_queue_async_work(env, req->task))

  return NULL;
}

NAPI_INIT() {
  if (hid_init() != 0) {
    napi_throw_error(env, NULL, "Failed to hid_init");
    return;
  }

  NAPI_EXPORT_NAMED_FUNCTION("enumerate", napi_hid_enumerate);
  NAPI_EXPORT_NAMED_FUNCTION("open", napi_hid_open);
  NAPI_EXPORT_NAMED_FUNCTION("open_path", napi_hid_open_path);
  NAPI_EXPORT_NAMED_FUNCTION("write", napi_hid_write);
  NAPI_EXPORT_NAMED_FUNCTION("read_timeout", napi_hid_read_timeout);
  NAPI_EXPORT_NAMED_FUNCTION("read", napi_hid_read);
  NAPI_EXPORT_NAMED_FUNCTION("read_timeout_async", napi_hid_read_timeout_async);
  NAPI_EXPORT_NAMED_FUNCTION("read_async", napi_hid_read_async);
  NAPI_EXPORT_NAMED_FUNCTION("set_nonblocking", napi_hid_set_nonblocking);
  NAPI_EXPORT_NAMED_FUNCTION("send_feature_report", napi_hid_send_feature_report);
  NAPI_EXPORT_NAMED_FUNCTION("get_feature_report", napi_hid_get_feature_report);
  NAPI_EXPORT_NAMED_FUNCTION("get_feature_report_async", napi_hid_get_feature_report_async);


  NAPI_STATUS_THROWS(napi_add_env_cleanup_hook(env, (void *)hid_exit, NULL));
}

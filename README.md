# `hid`

> Low-level bindings for USB/Bluetooth hidapi

## Usage

```js
var hid = require('hid')

// List all HID devices
console.log(hid.enumerate(0, 0))
```

## API

### `hid.enumerate(vendor_id, product_id)`

Enumerate all connected HID devices, filtered by `vendor_id` and `product_id`.
Setting these to `0` disables filtering on either.

## Install

```sh
npm install hid
```

## License

[ISC](LICENSE)

const hid = require('.')

const devices = hid.enumerate(0, 0)

var device = hid.open(devices[0].vendor_id, devices[0].product_id, devices[0].serial_number)

hid.set_nonblocking(device, 1)

const buf = Buffer.from([0x0, 0x1])
console.log(hid.read(device, buf))

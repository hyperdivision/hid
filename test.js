const test = require('tape')
const hid = require('.')

test('fail', function (assert) {
  assert.throws(() => hid.enumerate(''))
  assert.throws(() => hid.enumerate('', 0))
  assert.throws(() => hid.enumerate(0, ''))

  assert.throws(() => hid.open())
  assert.throws(() => hid.open(''))
  assert.throws(() => hid.open('', 0))
  assert.throws(() => hid.open(0, ''))
  assert.throws(() => hid.open(0, '', 0))
  assert.throws(() => hid.open('', 0, 0))
  assert.throws(() => hid.open(0, 0, 0))
  assert.throws(() => hid.write())
  assert.throws(() => hid.write(null))
  assert.throws(() => hid.write(null, Buffer.alloc(0)))

  assert.end()
})

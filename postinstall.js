var os = require('os')
var fs = require('fs')
var path = require('path')
var proc = require('child_process')
var ini = require('ini')

var release = path.join(__dirname, 'build/Release')
var debug = path.join(__dirname, 'build/Debug')
var tmp = path.join(__dirname, 'tmp')
var build = fs.existsSync(release) ? release : debug
var arch = process.env.PREBUILD_ARCH || os.arch()

switch (os.platform()) {
  case 'win32':
    buildWindows()
    break

  case 'darwin':
    buildDarwin()
    break

  case 'freebsd':
  case 'openbsd':
  default:
    buildUnix()
    break
}

function buildWindows () {
  var lib = path.join(__dirname, 'lib/libhidapi-' + arch + '.dll')
  var dst = path.join(build, 'libhidapi.dll')
  if (fs.existsSync(dst)) return
  copy(lib, dst, function (err) {
    if (err) throw err
  })
}

function buildUnix () {
  var lib = fs.realpathSync(path.join(__dirname, 'lib/libhidapi-hidraw-' + arch + '.so'))

  var la = ini.decode(fs.readFileSync(path.join(tmp, 'lib/libhidapi-hidraw.la')).toString())
  var dst = path.join(build, la.dlname)

  if (fs.existsSync(dst)) return
  copy(lib, dst, function (err) {
    if (err) throw err
  })
}

function buildDarwin () {
  var lib = path.join(__dirname, 'lib/libhidapi-' + arch + '.dylib')
  var dst = path.join(build, 'libhidapi.dylib')
  if (fs.existsSync(dst)) return
  copy(lib, dst, function (err) {
    if (err) throw err
    proc.exec('install_name_tool -id "@loader_path/libhidapi.dylib" libhidapi.dylib', { cwd: build }, function (err) {
      if (err) throw err
      proc.exec('install_name_tool -change "' + lib + '" "@loader_path/libhidapi.dylib" hidapi.node', { cwd: build }, function (err) {
        if (err) throw err
      })
    })
  })
}

function copy (a, b, cb) {
  fs.stat(a, function (err, st) {
    if (err) return cb(err)
    fs.readFile(a, function (err, buf) {
      if (err) return cb(err)
      fs.writeFile(b, buf, function (err) {
        if (err) return cb(err)
        fs.chmod(b, st.mode, cb)
      })
    })
  })
}

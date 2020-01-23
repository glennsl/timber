# timber
Opinionated logging framework based on [@dbuenzli/logs](https://github.com/dbuenzli/logs)

![Screenshot](https://github.com/glennsl/timber/blob/master/assets/screenshot.png)

### Features
- Colorized output (except on Windows, because Windows)
- Namespaces, filterable and colorized by hash
- Delta timestamp showing time since previous message
- Console and file reporter out-of-the-box
- TRACE level
- Very simple API

### Example

```reason
Timber.App.enable();
Timber.App.setLevel(Timber.Level.debug);
Timber.App.setLogFile("test.log");

module Log = (val Timber.Log.withNamespace("Timber"));

Log.trace("Thois won't be logged");

Log.infof(m => m("Formatting and lazy evaluation %i", Random.int(100)));

Log.error("Something went horribly wrong!");
```

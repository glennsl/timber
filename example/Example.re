Timber.App.enablePrinting();
Timber.App.setLogFile("test.log");

module Log = (val Timber.withNamespace("Timber.Example"));

Log.debug("debug");
Log.info("info");
Log.warn("warn");
Log.error("error");

Log.debugf(m => m("debugf - %f", Unix.time()));
Log.infof(m => m("infof - %f", Unix.time()));
Log.warnf(m => m("warnf - %f", Unix.time()));
Log.errorf(m => m("errorf - %f", Unix.time()));

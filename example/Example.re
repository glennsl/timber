Timber.App.enable();
Timber.App.setLevel(Timber.Level.trace);
Timber.App.setLogFile("test.log");

{
  module Log = (val Timber.Log.withNamespace("Timber"));

  Log.trace("trace");
  Log.debug("debug");
  Log.info("info");
  Log.warn("warn");
  Log.error("error");
};

{
  module Log = (val Timber.Log.withNamespace("Timber.Formatters"));

  Log.tracef(m => m("tracef - User time: %fs", Unix.times().tms_utime));
  Log.debugf(m => m("debugf - System time: %fs", Unix.times().tms_stime));
  Log.infof(m => m("infof - UID %i", Unix.getuid()));
  Log.warnf(m => m("warnf - PID: %i", Unix.getpid()));
  Log.errorf(m => m("errorf - Since 1970: %fs", Unix.gettimeofday()));
};

{
  Timber.Log.perf("perf", () => Unix.sleepf(0.25));
};

{
  module Log = (val Timber.Log.withNamespace("Timber.Fn"));

  Log.fn("Unix.sleepf", Unix.sleepf, 0.25);
  Log.fn("Unix.getppid", ~pp=string_of_int, Unix.getppid, ());
};

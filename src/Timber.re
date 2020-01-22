module Internal = {
  let log = (~namespace="Global", (level, maybeMicrolevel), msgf) =>
    Logs.msg(level, m =>
      if (Level.isEnabled((level, maybeMicrolevel))
          && Namespace.isEnabled(namespace)) {
        let tags =
          Logs.Tag.(
            empty
            |> add(Namespace.tag, namespace)
            |> add(DeltaTime.tag, DeltaTime.get())
          );

        let tags =
          switch (maybeMicrolevel) {
          | Some(microlevel) =>
            Logs.Tag.add(Level.Microlevel.tag, microlevel, tags)
          | None => tags
          };

        msgf(m(~header=?None, ~tags));
      }
    );

  let fn = (~namespace=?, name, f, ~pp=?, x) => {
    log(~namespace?, Level.trace, m => m("Entering %s", name));
    let ret = f(x);
    log(~namespace?, Level.trace, m =>
      switch (pp) {
      | Some(pp) => m("Exited %s with %s", name, pp(ret))
      | None => m("Exited %s%s", name, "") // This weirdness is because the turened formatter must have the same type for both branches
      }
    );
    ret;
  };
};

type msgf('a, 'b) = (format4('a, Format.formatter, unit, 'b) => 'a) => 'b;

module type Logger = {
  let errorf: msgf(_, unit) => unit;
  let error: string => unit;
  let warnf: msgf(_, unit) => unit;
  let warn: string => unit;
  let infof: msgf(_, unit) => unit;
  let info: string => unit;
  let debugf: msgf(_, unit) => unit;
  let debug: string => unit;
  let tracef: msgf(_, unit) => unit;
  let trace: string => unit;
  let fn: (string, 'a => 'b, ~pp: 'b => string=?, 'a) => 'b;
};

module Level = Level;

module Log = {
  let perf = (msg, f) => {
    let startTime = Unix.gettimeofday();
    let ret = f();
    let endTime = Unix.gettimeofday();
    Internal.log(~namespace="Performance Measurement", Level.perf, m =>
      m("%s took %fs", msg, endTime -. startTime)
    );
    ret;
  };

  let withNamespace = namespace => {
    let logf = (level, msgf) => Internal.log(~namespace, level, msgf);
    let log = (level, msg) => logf(level, m => m("%s", msg));

    module Log = {
      let errorf = msgf => logf(Level.error, msgf);
      let error = log(Level.error);
      let warnf = msgf => logf(Level.warn, msgf);
      let warn = log(Level.warn);
      let infof = msgf => logf(Level.info, msgf);
      let info = log(Level.info);
      let debugf = msgf => logf(Level.debug, msgf);
      let debug = log(Level.debug);
      let tracef = msgf => logf(Level.trace, msgf);
      let trace = log(Level.trace);
      let fn = (name, f, ~pp=?, x) =>
        Internal.fn(~namespace, name, f, ~pp?, x);
    };

    ((module Log): (module Logger));
  };
};

module App = {
  let isEnabled = Reporter.isEnabled;
  let isLevelEnabled = Level.isEnabled;
  let isNamespaceEnabled = Namespace.isEnabled;

  let enable = () => Reporter.(set(all));
  let disable = () => Reporter.(set(none));

  let disableColors = Reporter.Console.disableColors;
  let setLevel = Level.set;
  let setLogFile = Reporter.File.setLogFile;
  let setNamespaceFilter = Namespace.setFilter;
};

// init
let () =
  if (Sys.win32) {
    Reporter.Console.init(~style_renderer=`None);
  } else {
    Reporter.Console.init(~style_renderer=`Ansi_tty);
  };

// default to only log messages at log level Info and above
Level.set(Level.info);

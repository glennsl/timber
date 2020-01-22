// TODO: Remove after 4.08
module Option = {
  let bind = (o, f) =>
    switch (o) {
    | Some(x) => f(x)
    | None => None
    };

  let iter = f =>
    fun
    | Some(x) => f(x)
    | None => ();
};

module Utility = {
  let formatTime = (tm: Unix.tm) =>
    Printf.sprintf(
      "%i-%02i-%02iT%02i:%02i:%02iZ",
      1900 + tm.tm_year,
      tm.tm_mon + 1,
      tm.tm_mday,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec,
    );
};

module Constants = {
  let colors = [|
    // `Black
    `Blue,
    `Cyan,
    `Green,
    `Magenta,
    `Red,
    //`White,
    `Yellow,
    `Hi(`Black),
    `Hi(`Blue),
    `Hi(`Cyan),
    `Hi(`Green),
    `Hi(`Magenta),
    `Hi(`Red),
    `Hi(`White),
    `Hi(`Yellow),
  |];
};

module Namespace = {
  let pickColor = namespace => {
    let index = Hashtbl.hash(namespace) mod Array.length(Constants.colors);
    Constants.colors[index];
  };

  let pp = ppf => Format.pp_print_string(ppf);
  let tag = Logs.Tag.def("namespace", pp);

  let includes = ref([]);
  let excludes = ref([]);

  let isEnabled = namespace => {
    let test = re => Re.execp(re, namespace);
    let included = includes^ == [] || List.exists(test, includes^);
    let excluded = List.exists(test, excludes^);

    included && !excluded;
  };

  let setFilter = filter => {
    let filters =
      filter |> String.split_on_char(',') |> List.map(String.trim);

    let (incs, excs) =
      filters
      |> List.fold_left(
           ((includes, excludes), filter) =>
             if (filter == "") {
               (includes, excludes);
             } else if (filter.[0] == '-') {
               let filter =
                 String.sub(filter, 1, String.length(filter) - 2)
                 |> Re.Glob.glob
                 |> Re.compile;

               (includes, [filter, ...excludes]);
             } else {
               let filter = filter |> Re.Glob.glob |> Re.compile;

               ([filter, ...includes], excludes);
             },
           ([], []),
         );

    includes := incs;
    excludes := excs;
  };
};

module DeltaTime = {
  let lastTimestamp = ref(Unix.gettimeofday());

  let get = () => {
    let last = lastTimestamp^;
    let now = Unix.gettimeofday();
    lastTimestamp := now;
    now -. last;
  };

  let pp = (ppf, dt) =>
    dt > 10.
      ? Fmt.pf(ppf, "%+6.2fs", dt) : Fmt.pf(ppf, "%+5.0fms", dt *. 1000.);

  let tag = Logs.Tag.def("time", pp);
};

module Microlevel = {
  type t =
    | Trace
    | Perf;

  let toString =
    fun
    | Trace => "TRACE"
    | Perf => "PERF";

  let pp = (ppf, level) => Fmt.pf(ppf, "%s", toString(level));

  let tag = Logs.Tag.def("microlevel", pp);
};

module Level = {
  type t = (Logs.level, option(Microlevel.t));

  let error = (Logs.Error, None);
  let warn = (Logs.Warning, None);
  let info = (Logs.Info, None);
  let debug = (Logs.Debug, None);
  let trace = (Logs.Debug, Some(Microlevel.Trace));
  let perf = (Logs.Debug, Some(Microlevel.Perf));

  let toString =
    fun
    | (Logs.App, _) => ""
    | (Logs.Error, _) => "ERROR"
    | (Logs.Warning, _) => "WARN"
    | (Logs.Info, _) => "INFO"
    | (Logs.Debug, None) => "DEBUG"
    | (Logs.Debug, Some(microlevel)) => Microlevel.toString(microlevel);

  let toColors =
    fun
    | (Logs.App, _) => (`Black, `Cyan)
    | (Logs.Error, _) => (`Black, `Red)
    | (Logs.Warning, _) => (`Black, `Yellow)
    | (Logs.Info, _) => (`Black, `Blue)
    | (Logs.Debug, None) => (`Black, `Green)
    | (Logs.Debug, Some(_)) => (`Black, `White);

  let pp = (ppf, level: t) =>
    Fmt.pf(ppf, "%-7s", "[" ++ toString(level) ++ "]");

  let pp_styled = (ppf, level) => {
    let (fg, bg) = toColors(level);

    let style = Fmt.(styled(`Fg(fg), styled(`Bg(bg), pp)));
    Fmt.pf(ppf, "%a", style, level);
  };
};

module Internal = {
  let logFileChannel = ref(None);

  // We use `native_error` instead of `prerr` / default formatter to work around:
  // https://github.com/ocaml/ocaml/issues/9252
  external prerr_native: string => unit = "timber_prerr_native";
  let consoleFormatter = {
    let buffer = Buffer.create(0);

    Format.make_formatter(
      Buffer.add_substring(buffer),
      () => {
        prerr_native(Buffer.contents(buffer));
        Buffer.clear(buffer);
      },
    );
  };

  let reporter = {
    let consoleReporter = {
      Logs.{
        report: (_src, level, ~over, k, msgf) => {
          let k = _ => {
            over();
            k();
          };

          msgf((~header as _=?, ~tags=?, fmt) => {
            let namespace = Option.bind(tags, Tag.find(Namespace.tag));
            let microlevel = Option.bind(tags, Tag.find(Microlevel.tag));
            let dt = Option.bind(tags, Tag.find(DeltaTime.tag));
            let color = Namespace.pickColor(namespace);
            let style = pp => Fmt.(styled(`Fg(color), pp));

            Format.kfprintf(
              k,
              consoleFormatter,
              "%a %a %a : @[" ^^ fmt ^^ "@]@.",
              Level.pp_styled,
              (level, microlevel),
              ppf => Option.iter(DeltaTime.pp(ppf)),
              dt,
              ppf => Option.iter(Fmt.pf(ppf, "%a", style(Namespace.pp))),
              namespace,
            );
          });
        },
      };
    };

    let fileReporter =
      Logs.{
        report: (_src, level, ~over, k, msgf) => {
          let k = _ => {
            over();
            k();
          };

          switch (logFileChannel^) {
          | Some(channel) =>
            let ppf = Format.formatter_of_out_channel(channel);

            msgf((~header=?, ~tags=?, fmt) => {
              let namespace = Option.bind(tags, Tag.find(Namespace.tag));
              let microlevel = Option.bind(tags, Tag.find(Microlevel.tag));

              Format.kfprintf(
                k,
                ppf,
                "%a %a @[" ^^ fmt ^^ "@]@.",
                Level.pp,
                (level, microlevel),
                ppf => Option.iter(Namespace.pp(ppf)),
                namespace,
              );
            });

          | None => k()
          };
        },
      };

    Logs.{
      report: (src, level, ~over, k, msgf) => {
        let kret =
          consoleReporter.report(src, level, ~over=() => (), k, msgf);
        fileReporter.report(src, level, ~over, () => kret, msgf);
      },
    };
  };

  let log = (~namespace="Global", (level, maybeMicrolevel), msgf) =>
    Logs.msg(level, m =>
      if (Namespace.isEnabled(namespace)) {
        let tags =
          Logs.Tag.(
            empty
            |> add(Namespace.tag, namespace)
            |> add(DeltaTime.tag, DeltaTime.get())
          );

        let tags =
          switch (maybeMicrolevel) {
          | Some(microlevel) =>
            Logs.Tag.add(Microlevel.tag, microlevel, tags)
          | None => tags
          };

        msgf(m(~header=?None, ~tags));
      }
    );

  let fn = (~namespace=?, name, f, x) => {
    log(~namespace?, Level.trace, m => m("Entering %s", name));
    let ret = f(x);
    log(~namespace?, Level.trace, m => m("Exited %s", name));
    ret;
  };
};

let isPrintingEnabled = () => Logs.reporter() !== Logs.nop_reporter;
let isDebugLoggingEnabled = () =>
  Logs.Src.level(Logs.default) == Some(Logs.Debug);
let isNamespaceEnabled = Namespace.isEnabled;

type msgf('a, 'b) = (format4('a, Format.formatter, unit, 'b) => 'a) => 'b;

let info = msg => Internal.log(Level.info, m => m("%s", msg));
let debug = msgf => Internal.log(Level.debug, m => m("%s", msgf()));
let error = msg => Internal.log(Level.error, m => m("%s", msg));

let perf = (msg, f) => {
  let startTime = Unix.gettimeofday();
  let ret = f();
  let endTime = Unix.gettimeofday();
  Internal.log(~namespace="Performance Mesaurement", Level.perf, m => m("%s took %fs", msg, endTime -. startTime));
  ret;
};

let fn = (name, f, x) => Internal.fn(~namespace=?None, name, f, x);

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
  let fn: (string, 'a => 'b, 'a) => 'b;
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
    let fn = (name, f, x) => Internal.fn(~namespace, name, f, x);
  };

  ((module Log): (module Logger));
};

module App = {
  let disableColors = () =>
    Fmt_tty.setup_std_outputs(~style_renderer=`None, ());

  let enablePrinting = () => Logs.set_reporter(Internal.reporter);

  let enableDebugLogging = () =>
    Logs.Src.set_level(Logs.default, Some(Logs.Debug));

  let setLogFile = (~truncate=false, path) => {
    // Close previous log file, if any
    Option.iter(close_out, Internal.logFileChannel^);

    // Open new log file
    let mode =
      truncate
        ? [Open_append, Open_creat, Open_trunc, Open_text]
        : [Open_append, Open_creat, Open_text];
    let channel = open_out_gen(mode, 0o666, path);

    // Write log file header
    Printf.fprintf(
      channel,
      "\n--\nLog started at %s\n--\n\n%!",
      Unix.time() |> Unix.gmtime |> Utility.formatTime,
    );

    // Set new log file to be used by logging functions
    Internal.logFileChannel := Some(channel);
  };

  let setNamespaceFilter = Namespace.setFilter;
};

// init
let () =
  if (Sys.win32) {
    Fmt.set_style_renderer(Internal.consoleFormatter, `None);
  } else {
    Fmt.set_style_renderer(Internal.consoleFormatter, `Ansi_tty);
  };

Logs.set_level(Some(Logs.Info));

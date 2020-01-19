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
  let pp = (ppf, dt) =>
    dt > 10.
      ? Fmt.pf(ppf, "%+6.2fs", dt) : Fmt.pf(ppf, "%+5.0fms", dt *. 1000.);

  let tag = Logs.Tag.def("time", pp);
};

type msgf('a, 'b) = (format4('a, Format.formatter, unit, 'b) => 'a) => 'b;

let logFileChannel = ref(None);

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

          Format.kfprintf(
            k,
            ppf,
            "%a %a @[" ^^ fmt ^^ "@]@.",
            pp_header,
            (level, header),
            ppf => Option.iter(Namespace.pp(ppf)),
            namespace,
          );
        });

      | None => k()
      };
    },
  };

let consoleReporter = {
  let pp_level = (ppf, level) => {
    let str =
      switch (level) {
      | Logs.App => ""
      | Logs.Error => "ERROR"
      | Logs.Warning => "WARN"
      | Logs.Info => "INFO"
      | Logs.Debug => "DEBUG"
      };

    Fmt.pf(ppf, "%-7s", "[" ++ str ++ "]");
  };

  let pp_level_styled = (ppf, level) => {
    let (fg, bg) =
      switch (level) {
      | Logs.App => (`White, `Cyan)
      | Logs.Error => (`White, `Red)
      | Logs.Warning => (`White, `Yellow)
      | Logs.Info => (`Black, `Blue)
      | Logs.Debug => (`Black, `Green)
      };

    let style = Fmt.(styled(`Fg(fg), styled(`Bg(bg), pp_level)));
    Fmt.pf(ppf, "%a", style, level);
  };

  let buffer = Buffer.create(0);
  let formatter =
    Format.make_formatter(
      Buffer.add_substring(buffer),
      () => {
        // We use `Console.log` instead of `print_endline` / default formatter to work around:
        // https://github.com/ocaml/ocaml/issues/9252
        Console.err(Buffer.contents(buffer));
        Buffer.clear(buffer);
      },
    );

  Logs.{
    report: (_src, level, ~over, k, msgf) => {
      let k = _ => {
        over();
        k();
      };

      msgf((~header as _=?, ~tags=?, fmt) => {
        let namespace = Option.bind(tags, Tag.find(Namespace.tag));
        let dt = Option.bind(tags, Tag.find(DeltaTime.tag));
        let color = Namespace.pickColor(namespace);
        let style = pp => Fmt.(styled(`Fg(color), pp));

        Format.kfprintf(
          k,
          formatter,
          "%a %a %a : @[" ^^ fmt ^^ "@]@.",
          pp_level_styled,
          level,
          ppf => Option.iter(DeltaTime.pp(ppf)),
          dt,
          ppf => Option.iter(Fmt.pf(ppf, "%a", style(Namespace.pp))),
          namespace,
        );
      });
    },
  };
};

let reporter =
  Logs.{
    report: (src, level, ~over, k, msgf) => {
      let kret = consoleReporter.report(src, level, ~over=() => (), k, msgf);
      fileReporter.report(src, level, ~over, () => kret, msgf);
    },
  };

let isPrintingEnabled = () => Logs.reporter() !== Logs.nop_reporter;
let isDebugLoggingEnabled = () =>
  Logs.Src.level(Logs.default) == Some(Logs.Debug);
let isNamespaceEnabled = Namespace.isEnabled;

let lastLogTime = ref(Unix.gettimeofday());

let log = (~namespace="Global", level, msgf) =>
  Logs.msg(level, m =>
    if (Namespace.isEnabled(namespace)) {
      let now = Unix.gettimeofday();
      let tags =
        Logs.Tag.(
          empty
          |> add(Namespace.tag, namespace)
          |> add(DeltaTime.tag, now -. lastLogTime^)
        );

      msgf(m(~header=?None, ~tags));

      lastLogTime := now;
    }
  );

let info = msg => log(Logs.Info, m => m("%s", msg));
let debug = msgf => log(Logs.Debug, m => m("%s", msgf()));
let error = msg => log(Logs.Error, m => m("%s", msg));

let perf = (msg, f) => {
  let startTime = Unix.gettimeofday();
  let ret = f();
  let endTime = Unix.gettimeofday();
  log(Logs.Debug, m => m("[PERF] %s took %fs", msg, endTime -. startTime));
  ret;
};

let fn' = (~namespace=?, name, f, x) => {
  log(~namespace?, Logs.Debug, m => m("Entering %s", name));
  let ret = f(x);
  log(~namespace?, Logs.Debug, m => m("Exited %s", name));
  ret;
};

let fn = (name, f, x) => fn'(~namespace=?None, name, f, x);

module type Logger = {
  let errorf: msgf(_, unit) => unit;
  let error: string => unit;
  let warnf: msgf(_, unit) => unit;
  let warn: string => unit;
  let infof: msgf(_, unit) => unit;
  let info: string => unit;
  let debugf: msgf(_, unit) => unit;
  let debug: string => unit;
  let fn: (string, 'a => 'b, 'a) => 'b;
};

let withNamespace = namespace => {
  let logf = (level, msgf) => log(~namespace, level, msgf);
  let log = (level, msg) => logf(level, m => m("%s", msg));

  module Log = {
    let errorf = msgf => logf(Logs.Error, msgf);
    let error = log(Logs.Error);
    let warnf = msgf => logf(Logs.Warning, msgf);
    let warn = log(Logs.Warning);
    let infof = msgf => logf(Logs.Info, msgf);
    let info = log(Logs.Info);
    let debugf = msgf => logf(Logs.Debug, msgf);
    let debug = log(Logs.Debug);
    let fn = (name, f, x) => fn'(~namespace, name, f, x);
  };

  ((module Log): (module Logger));
};

module App = {
  let disableColors = () =>
    Fmt_tty.setup_std_outputs(~style_renderer=`None, ());

  let enablePrinting = () => Logs.set_reporter(reporter);

  let enableDebugLogging = () =>
    Logs.Src.set_level(Logs.default, Some(Logs.Debug));

  let setLogFile = (~truncate=false, path) => {
    // Close previous log file, if any
    Option.iter(close_out, logFileChannel^);

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
    logFileChannel := Some(channel);
  };

  let setNamespaceFilter = Namespace.setFilter;
};

// init
let () =
  if (Sys.win32) {
    Fmt_tty.setup_std_outputs(~style_renderer=`None, ());
  } else {
    Fmt_tty.setup_std_outputs(~style_renderer=`Ansi_tty, ());
  };

Logs.set_level(Some(Logs.Info));

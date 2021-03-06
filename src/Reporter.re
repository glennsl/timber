open Utility;

module Console = {
  // We use `native_error` instead of `prerr` / default formatter to work around:
  // https://github.com/ocaml/ocaml/issues/9252
  external prerr_native: string => unit = "timber_prerr_native";
  let formatter = {
    let buffer = Buffer.create(0);

    Format.make_formatter(
      Buffer.add_substring(buffer),
      () => {
        prerr_native(Buffer.contents(buffer));
        Buffer.clear(buffer);
      },
    );
  };

  let init = (~style_renderer) =>
    Fmt.set_style_renderer(formatter, style_renderer);

  let disableColors = () => Fmt.set_style_renderer(formatter, `None);

  let getDeltaTime = DeltaTime.generator();

  let reporter =
    Logs.{
      report: (_src, level, ~over, k, msgf) => {
        let k = _ => {
          over();
          k();
        };

        msgf((~header as _=?, ~tags=?, fmt) => {
          let tags = Option.get(tags);
          let namespace = Tag.get(Namespace.tag, tags);
          let microlevel = Tag.find(Level.Microlevel.tag, tags);
          let level = (level, microlevel);
          let color = Namespace.pickColor(namespace);
          let style = pp => Fmt.(styled(`Fg(color), pp));

          Format.kfprintf(
            k,
            formatter,
            "%a %a %a : @[" ^^ fmt ^^ "@]@.",
            Level.pp_styled,
            level,
            DeltaTime.pp,
            getDeltaTime(),
            ppf => Fmt.pf(ppf, "%a", style(Namespace.pp)),
            namespace,
          );
        });
      },
    };
};

module File = {
  let channel = ref(None);

  let setLogFile = (~truncate=false, path) => {
    // Close previous log file, if any
    Option.iter(close_out, channel^);

    // Open new log file
    let mode =
      truncate
        ? [Open_append, Open_creat, Open_trunc, Open_text]
        : [Open_append, Open_creat, Open_text];
    let chan = open_out_gen(mode, 0o666, path);

    // Write log file header
    Printf.fprintf(
      chan,
      "\n--\nLog started at %s\n--\n\n%!",
      Unix.time() |> Unix.gmtime |> Time.toString,
    );

    // Set new log file to be used by logging functions
    channel := Some(chan);
  };

  let getDeltaTime = DeltaTime.generator();

  let reporter =
    Logs.{
      report: (_src, level, ~over, k, msgf) => {
        let k = _ => {
          over();
          k();
        };

        switch (channel^) {
        | Some(channel) =>
          let ppf = Format.formatter_of_out_channel(channel);

          msgf((~header as _=?, ~tags=?, fmt) => {
            let tags = Option.get(tags);
            let namespace = Tag.get(Namespace.tag, tags);
            let microlevel = Tag.find(Level.Microlevel.tag, tags);
            let level = (level, microlevel);

            Format.kfprintf(
              k,
              ppf,
              "%a %a %a : @[" ^^ fmt ^^ "@]@.",
              Level.pp,
              level,
              DeltaTime.pp,
              getDeltaTime(),
              Namespace.pp,
              namespace,
            );
          });

        | None => k()
        };
      },
    };
};

let none = Logs.nop_reporter;
let console = Console.reporter;
let file = File.reporter;

let all =
  Logs.{
    report: (src, level, ~over, k, msgf) => {
      let kret = console.report(src, level, ~over=() => (), k, msgf);
      file.report(src, level, ~over, () => kret, msgf);
    },
  };

let isEnabled = () => Logs.reporter() !== Logs.nop_reporter;
let set = Logs.set_reporter;

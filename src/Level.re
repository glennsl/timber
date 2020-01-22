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

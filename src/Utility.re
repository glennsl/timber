module Time = {
  let toString = (tm: Unix.tm) =>
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

  let map = (f, o) =>
    switch (o) {
    | Some(x) => Some(f(x))
    | None => None
    };

  let get =
    fun
    | Some(x) => x
    | None => invalid_arg("Option.get");
};

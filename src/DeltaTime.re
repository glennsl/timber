let last = ref(Unix.gettimeofday());

let get = () => {
  let now = Unix.gettimeofday();
  let delta = now -. last^;
  last := now;
  delta;
};

let pp = (ppf, dt) =>
  if (dt > 10.) {
    Fmt.pf(ppf, "%+6.2fs", dt);
  } else {
    Fmt.pf(ppf, "%+5.0fms", dt *. 1000.);
  };

let tag = Logs.Tag.def("time", pp);

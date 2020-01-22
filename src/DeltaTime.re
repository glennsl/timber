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

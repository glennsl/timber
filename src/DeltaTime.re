let generator = () => {
  let last = ref(Unix.gettimeofday());

  () => {
    let now = Unix.gettimeofday();
    let delta = now -. last^;
    last := now;
    delta;
  };
};

let pp = (ppf, dt) =>
  if (dt > 36000.) {
    Fmt.pf(ppf, "%+6.1fh", dt /. 3600.);
  } else if (dt > 600.) {
    Fmt.pf(ppf, "%+6.1fm", dt /. 60.);
  } else if (dt > 100.) {
    Fmt.pf(ppf, "%+6.1fs", dt);
  } else if (dt > 10.) {
    Fmt.pf(ppf, "%+6.2fs", dt);
  } else {
    Fmt.pf(ppf, "%+5.0fms", dt *. 1000.);
  };

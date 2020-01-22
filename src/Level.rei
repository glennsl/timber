module Microlevel: {
  type t;
  let tag: Logs.Tag.def(t);
};

type t = (Logs.level, option(Microlevel.t));

let error: t;
let warn: t;
let info: t;
let debug: t;
let trace: t;
let perf: t;

let pp: Fmt.t(t);
let pp_styled: Fmt.t(t);

let isEnabled: t => bool;
let set: t => unit;

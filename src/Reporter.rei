module Console: {
  let init: (~style_renderer: Fmt.style_renderer) => unit;
  let disableColors: unit => unit;
};

module File: {let setLogFile: (~truncate: bool=?, string) => unit;};

let none: Logs.reporter;
let console: Logs.reporter;
let file: Logs.reporter;
let all: Logs.reporter;

let isEnabled: unit => bool;
let setReporter: Logs.reporter => unit;
let currentLevel: unit => option(Level.t);
let setLevel: Level.t => unit;

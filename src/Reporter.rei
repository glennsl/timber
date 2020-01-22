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
let set: Logs.reporter => unit;

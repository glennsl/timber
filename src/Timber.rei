type msgf('a, 'b) = (format4('a, Format.formatter, unit, 'b) => 'a) => 'b;

let isPrintingEnabled: unit => bool;
let isDebugLoggingEnabled: unit => bool;
let isNamespaceEnabled: string => bool;

let info: string => unit;
let debug: (unit => string) => unit;
let error: string => unit;
let perf: (string, unit => 'a) => 'a;
let fn: (string, 'a => 'b, 'a) => 'b;

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

let withNamespace: string => (module Logger);

module App: {
  // These function should only be used by the application, not libraries

  let disableColors: unit => unit;
  let enablePrinting: unit => unit;
  let enableDebugLogging: unit => unit;
  let setLogFile: (~truncate: bool=?, string) => unit;

  /**
   * setNamespaceFilter(filters)
   *
   * where `filter` is a comma-separated list of glob patterns to include,
   * optionally prefixed with a `-` to negate it. A blank string includes
   * everything, and is the default.
   *
   * E.g.
   *   setNamespaceFilter("Oni2.*, -Revery*")
   */
  let setNamespaceFilter: string => unit;
};

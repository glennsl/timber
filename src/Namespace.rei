let tag: Logs.Tag.def(string);

let pickColor: string => [ Fmt.color | `Hi(Fmt.color)];
let isEnabled: string => bool;
let setFilter: string => unit;

let pp: Fmt.t(string);
